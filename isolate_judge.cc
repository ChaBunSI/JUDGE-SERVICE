#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <iomanip>
#include <chrono>

// AWS 관련 라이브러리
#include "judge_sqs.h"

// 채점 관련 라이브러리
#include "isolate_judge.h"
#include "judge_task.h"

// 워커 (채점 프로세스)의 id 및 샌드박스 경로
static int wid=-1;
std::string isolate_dir;

int main(int argc, char *argv[]) {
    wid = 0;
    isolate_dir = "/var/local/lib/isolate/" + std::to_string(wid) + "/box";
    //system(std::string("isolate --box-id=" + std::to_string(wid) + " --init").c_str());

    // if(argc < 2) { std::cerr << "no worker ids are given\n"; return -1; }
    // else if(argc == 2) {
    //     wid = atoi(argv[1]);
    //     isolate_dir = "/var/local/lib/isolate/" + std::to_string(wid) + "/box";
    //     system(std::string("isolate --box-id=" + std::to_string(wid) + " --init").c_str());
    // }

    // 채점 큐에서 꺼낸 제출 정보
    user_submission cur_sub;
    bool received_sub = false;

    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        Aws::String queueName = QUEUE_NAME;
        Aws::String queueUrl = QUEUE_URL;
        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = Aws::Region::AP_NORTHEAST_2;

        // 채점 큐에서 제출 정보 꺼내기
        received_sub = receiveMessage(cur_sub, clientConfig);
    }

    if(!received_sub) {
        Aws::ShutdownAPI(options);
        std::cerr << "Failed to receive message\n";
        return -1;
    } else {
        Aws::ShutdownAPI(options);
        std::cout << "Received the message successfully\n";
        return 0;
    }

    auto cur_config = lang_configs[cur_sub.lang];
    judge_info cur_judge_info;
    int compile_res = cur_sub.compile_code(cur_config);
    
    if(compile_res == COMPILE_ERROR) {
        cur_judge_info.res = CE;
    } else if(compile_res == FILE_CREATE_ERROR) {
        std::cerr << "Failed to make the file\n";
    }

    // 채점하기
    std::string cur_tc_dir = "testcases/" + std::to_string(cur_sub.problem_id) + "/";
    auto cur_tc_dir_iter = std::filesystem::directory_iterator(cur_tc_dir);

    // 테스트 케이스 개수 세기
    int cnt_tc = std::count_if(
        begin(cur_tc_dir_iter), end(cur_tc_dir_iter),
        [](const std::filesystem::directory_entry& e) {
            return e.is_regular_file();
        }
    ) / 2;

    std::vector<judge_info> judge_res(cnt_tc);
    
    // 문제의 테스트 케이스들과 실행 코드를 샌드박스 내로 복사
    std::string cp_tc = "cp Main " + cur_tc_dir + "/* " + isolate_dir;
    if(system(cp_tc.c_str()) < 0) {
        std::cerr << "Failed to copy the files\n";
        return -1;
    }

    // 샌드박스 내에서 테스트케이스 별로 제출된 코드 실행 후 결과 저장
    for(int i = 1; i <= cnt_tc; i++) {
        std::string tc_num = i < 10 ? "0" + std::to_string(i) : std::to_string(i);
        std::string cur_cmd = "isolate --stdin=" + tc_num + ".in --stdout=usr_" + tc_num + ".out --run " + cur_config.run_cmd;
        judge_info& cur_tc_judge_info = judge_res[i-1];

        int sandbox_exec_res = system(cur_cmd.c_str()); 

        cur_tc_judge_info.tc_id = i;

        if(sandbox_exec_res != 0) {
            std::cerr << "Sandbox Execution Error\n";
            cur_tc_judge_info.res = SE;
        }

        // TODO:샌드박스에서 결과 가져오기 (에러 등)

        std::ifstream usr_out(isolate_dir + "/usr_" + tc_num + ".out");
        std::ifstream tc_out(cur_tc_dir + "/" + tc_num + ".out");
        std::string tc_out_str, usr_out_str;

        while(std::getline(tc_out, tc_out_str) && std::getline(usr_out, usr_out_str)) {
            if(!strip_and_compare(tc_out_str, usr_out_str)) {
                cur_tc_judge_info.res = WA;
                break;
            } else cur_tc_judge_info.res = AC;
        }
        usr_out.close();
        tc_out.close();
    }
    print_statistics(judge_res);
    system(std::string("isolate --box-id=" + std::to_string(wid) + " --cleanup").c_str());

    Aws::ShutdownAPI(options);
    return 0;
}