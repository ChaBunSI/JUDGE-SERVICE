#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <iomanip>
#include <chrono>

// 채점 관련 라이브러리
#include "judge_worker.h"
#include "judge_task.h"
#include "judge_notify.h"
#include "judge_error.h"

// 워커 (채점 프로세스)의 id 및 샌드박스 경로
static int wid = 0;
std::string isolate_dir;

// worker 프로세스는 인자로 0~1 사이 숫자를 받는다.
// 0: C/C++가 아닌, 다른 프로그래밍 언어를 채점
// 1: C/C++를 채점
// 채점하는 프로그래밍 언어에 따라, 폴링할 큐의 이름과 URL이 달라 초기 설정 필요하다.

static Aws::String QUEUE_NAME;
static Aws::String QUEUE_URL;

int main(int argc, char *argv[])
{
    if(argc == 2) {
        int is_cpp = std::stoi(argv[1]);
        if(is_cpp == 0) {
            std::cout << "========================================\n";
            std::cout << "Worker Init: Supports languages other than C/C++\n";
            std::cout << "========================================\n";
            QUEUE_NAME = NotCPP_QUEUE_NAME;
            QUEUE_URL = NotCPP_QUEUE_URL;
        } else if(is_cpp == 1) {
            std::cout << "========================================\n";
            std::cout << "Worker Init: Supports C/C++\n";
            std::cout << "========================================\n";
            QUEUE_NAME = CPP_QUEUE_NAME;
            QUEUE_URL = CPP_QUEUE_URL;
        } else {
            std::cerr << "Usage: ./worker [0~1]\n";
            return -1;
        }
    } else {
        std::cerr << "Usage: ./worker [0~1]\n";
        return -1;
    }

    isolate_dir = "/var/local/lib/isolate/" + std::to_string(wid) + "/box";
    system(std::string("isolate --cg --box-id=" + std::to_string(wid) + " --init").c_str());

    // 채점 큐에서 꺼낸 제출 정보
    user_submission cur_sub;
    bool received_sub = false, deleted_msg = false;

    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        // 채점 큐에서 제출 정보 꺼내기
        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = Aws::Region::AP_NORTHEAST_2;
        while (1)
        {
            Aws::String messageReceiptHandle;
            received_sub = receiveMessageTaskQueue(cur_sub, clientConfig, messageReceiptHandle, QUEUE_URL);
            if (!received_sub)
            {
                std::cerr << "Failed to receive the message\n";
                sleep(1);
            }
            else
            {
                std::cout << "Received the message successfully\n";
                deleteMessageTaskQueue(messageReceiptHandle, clientConfig, QUEUE_URL);

                // 채점마다 필요한 변수들
                auto cur_config = lang_configs[cur_sub.lang];
                judge_info cur_judge_info;
                error_code process_res = NO_ERROR;
                bool error_occured = false;
                int tc_cnt = 0, ac_cnt = 0;

                cur_judge_info.res = NJ;

                // 해당 문제 or 문제의 테케 존재 여부 확인
                if (!std::filesystem::exists("../testcases/" + std::to_string(cur_sub.problem_id)))
                {
                    std::cerr << std::to_string(cur_sub.problem_id) << " does not exist\n";
                    cur_judge_info.res = NJ;
                    cur_judge_info.err_msg = std::to_string(cur_sub.problem_id) + " does not exist";
                    process_res = NO_FILE_ERROR;
                }
                else
                {
                    cur_sub.compile_code(cur_config);
                }

                if (process_res == COMPILE_ERROR)
                {
                    cur_judge_info.res = CE;
                }
                else if (process_res == FILE_CREATE_ERROR)
                {
                    std::cerr << "Failed to make the file\n";
                    cur_judge_info.res = NJ;
                    cur_judge_info.err_msg = "Judge Server Internal Error: Failed to create the file for the submission";
                }
                else if (process_res == NO_ERROR)
                {
                    std::string current_path = std::filesystem::current_path();
                    std::string submit_id = std::to_string(cur_sub.submit_id);

                    // 채점하기
                    std::string cur_tc_dir = "../testcases/" + std::to_string(cur_sub.problem_id) + "/";
                    auto cur_tc_dir_iter = std::filesystem::directory_iterator(cur_tc_dir);

                    // 테스트 케이스 개수 세기
                    tc_cnt = std::count_if(
                                     begin(cur_tc_dir_iter), end(cur_tc_dir_iter),
                                     [](const std::filesystem::directory_entry &e)
                                     {
                                         return e.is_regular_file();
                                     }) /
                                 2;

                    // 문제의 테스트 케이스들과 실행 코드를 샌드박스 내로 복사
                    std::string cp_tc = "cp Main" + cur_config.exec_ext + " " + cur_tc_dir + "/* " + isolate_dir;

                    if (system(cp_tc.c_str()) < 0)
                    {
                        std::cerr << "Failed to copy the files\n";
                        return -1;
                    }

                    // 샌드박스 내에서 테스트케이스 별로 제출된 코드 실행 후 결과 저장
                    for (int i = 1; i <= tc_cnt && !error_occured; i++)
                    {
                        std::string tc_num = std::to_string(i);
                        std::string cur_cmd = "isolate --cg --cg-mem=" + std::to_string(cur_config.get_max_mem(cur_sub.max_mem) * 1000) // MB -> KB로 변환
                                              + " --time=" + std::to_string(cur_config.get_max_time(cur_sub.max_time) / 1000.0)         // ms -> s로 변환
                                              + " --box-id=" + std::to_string(wid) + " --meta=" + submit_id + ".meta" + " --stdin=" + tc_num + ".in --stdout=usr_" + tc_num + ".out --run " + cur_config.run_cmd;
                        judge_info cur_tc_judge_info;

                        int sandbox_exec_res = system(cur_cmd.c_str());

                        cur_tc_judge_info.tc_id = i;

                        // 샌드박스에서 결과 가져오기 (에러 등)
                        std::ifstream usr_out(isolate_dir + "/usr_" + tc_num + ".out");
                        std::ifstream tc_out(cur_tc_dir + "/" + tc_num + ".out");
                        std::ifstream meta_out(submit_id + ".meta");
                        std::string tc_out_str, usr_out_str, meta_line;
                        std::map<std::string, std::string> meta_data;

                        // 메타 데이터 읽기
                        if (meta_out.is_open())
                        {
                            while (std::getline(meta_out, meta_line))
                            {
                                std::stringstream ss(meta_line);
                                std::string key, value;
                                std::getline(ss, key, ':');
                                std::getline(ss, value, ':');
                                meta_data[key] = value;
                            }

                            if (meta_data["exitcode"] != "0" && meta_data.count("status") > 0)
                            {
                                std::string status = meta_data["status"];
                                if (status == "TO")
                                {
                                    cur_tc_judge_info.res = TLE;
                                    cur_judge_info.res = TLE;
                                }
                                else if (status == "SG" || status == "RE")
                                {
                                    cur_tc_judge_info.res = RE;
                                    cur_judge_info.res = RE;
                                }
                                else if (status == "XX")
                                {
                                    cur_tc_judge_info.res = SE;
                                    cur_judge_info.res = SE;
                                }
                            } else if(meta_data["exitcode"] == "0") {
                                // 메타 데이터에서 실행 시간과 메모리 사용량 가져오기
                                cur_judge_info.time = std::max(cur_judge_info.time, static_cast<size_t>(1000 * std::stof(meta_data["time"])));
                                cur_judge_info.mem = std::max(cur_judge_info.mem, static_cast<size_t>(std::stod(meta_data["cg-mem"]))); 
                            }
                        }
                        else
                            std::cout << "meta_out is not open\n";

                        if (sandbox_exec_res != 0)
                        {
                            std::cerr << "Error occured at sandbox!\n";

                            if(cur_tc_judge_info.res == TLE || cur_tc_judge_info.res == RE || cur_tc_judge_info.res == SE)
                                cur_judge_info.res = cur_tc_judge_info.res;
                                if(cur_judge_info.res == RE && meta_data.count("exitsig") > 0) {
                                    cur_judge_info.err_msg = std::string("exitcode: " + meta_data["exitcode"] + " ") + SIGNALS[std::stoi(meta_data["exitsig"])];
                                }
                            else
                                cur_tc_judge_info.res = SE;
                            error_occured = true;

                            // 실시간 채점 현황에 에러 났다고 전송
                            sendToQueue(cur_sub.submit_id, i, cur_res_to_aws_string(cur_sub.submit_id, cur_sub.problem_id, tc_cnt, i, cur_tc_judge_info.res), clientConfig);
                            meta_out.close();
                            usr_out.close();
                            tc_out.close();
                            break;
                        }
                        
                        while (std::getline(tc_out, tc_out_str) && std::getline(usr_out, usr_out_str))
                        {
                            if (!strip_and_compare(tc_out_str, usr_out_str))
                            {
                                cur_tc_judge_info.res = WA;
                                break;
                            }
                            else
                                cur_tc_judge_info.res = AC;
                        }

                        if(cur_tc_judge_info.res == AC) ac_cnt++; 

                        if (std::getline(usr_out, usr_out_str))
                        {
                            cur_tc_judge_info.res = OLE;
                        }

                        sendToQueue(cur_sub.submit_id, i, cur_res_to_aws_string(cur_sub.submit_id, cur_sub.problem_id, tc_cnt, i, cur_tc_judge_info.res), clientConfig);
                        usr_out.close();
                        tc_out.close();
                        meta_out.close();

                        if(cur_tc_judge_info.res != AC) {
                            cur_judge_info.res = cur_tc_judge_info.res;
                            break;
                        }
                    }
                }
                
                if (process_res == NO_ERROR) {
                    print_statistics(cur_sub, cur_judge_info, tc_cnt, ac_cnt);
                    sendToQueue(cur_sub.submit_id, 0, final_res_to_aws_string(cur_sub.submit_id, cur_sub.problem_id, cur_judge_info.time, cur_judge_info.mem, cur_judge_info.res), clientConfig);
                    // TODO: 채점이 길어지면, 메세지가 삭제되지 않을 수 있음 -> 수정 필요
                    //deleteMessageTaskQueue(messageReceiptHandle, clientConfig, QUEUE_URL);
                }
                    
                std::string cleanup = "rm -rf " + isolate_dir + "/*";

                if (system(cleanup.c_str()) < 0) {
                    std::cerr << "Failed to clean up the files\n";
                    return -1;
                }

                std::string cleanup_meta = "rm " + std::to_string(cur_sub.submit_id) + ".meta";
                
                if (system(cleanup_meta.c_str()) < 0) {
                    std::cerr << "Failed to remove the meta file\n";
                    return -1;
                }
                
                publishToTopic(cur_sub.submit_id, judge_res_to_aws_string(cur_judge_info, cur_sub), clientConfig);
            }
        }
    }
    Aws::ShutdownAPI(options);
    system(std::string("isolate --box-id=" + std::to_string(wid) + " --cleanup").c_str());
    return 0;
}