#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <iomanip>
#include <chrono>

#include "isolate_judge.h"

static int wid=-1;
std::string isolate_dir;

bool strip_and_compare(std::string& str1, std::string& str2) {
    // Strip the string
    str1.erase(std::remove(str1.begin(), str1.end(), ' '), str1.end());
    str1.erase(std::remove(str1.begin(), str1.end(), '\n'), str1.end());
    str1.erase(std::remove(str1.begin(), str1.end(), '\r'), str1.end());
    str2.erase(std::remove(str2.begin(), str2.end(), ' '), str2.end());
    str2.erase(std::remove(str2.begin(), str2.end(), '\n'), str2.end());
    str2.erase(std::remove(str2.begin(), str2.end(), '\r'), str2.end());
    return str1 == str2;
}

void print_statistics(std::vector<judge_info>& judge_res) {
    int len = judge_res.size();
    int ac_cnt = 0, not_ac_cnt = 0;

    for(auto& e: judge_res) {
        if(e.res == AC) ac_cnt++;
        else not_ac_cnt++;
    }

    std::cout << "30461 Statistics: \n";
    std::cout << "AC: " << ac_cnt << "\n";
    std::cout << "WA: " << not_ac_cnt << "\n";
}

int main(int argc, char *argv[]) {
    if(argc < 2) { std::cerr << "no worker ids are given\n"; return -1; }
    else if(argc == 2) {
        wid = atoi(argv[1]);
        isolate_dir = "/var/local/lib/isolate/" + std::to_string(wid) + "/box";
        system(std::string("isolate --box-id=" + std::to_string(wid) + " --init").c_str());
    }

    // 채점 큐에서 꺼낸 제출 정보

    user_submission cur_sub(69170801, 30461, CPP, "pridom1118", R"(#include <bits/stdc++.h>
#define fastio cin.tie(0)->sync_with_stdio(0)

using namespace std;

int N, M, Q, W, P, ocean[2005][2005];

int main() {
    fastio;
    cin >> N >> M >> Q;

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < M; j++) {
            cin >> ocean[i][j];
            if(i > 0) ocean[i][j] += ocean[i-1][j];
            if(i && j) {
                ocean[i][j] += ocean[i-1][j-1];
                if(i > 1) ocean[i][j] -= ocean[i-2][j-1];
            }
        }
    }

    while(Q--) {
        cin >> W >> P;
        cout << ocean[W-1][P-1] << "\n";
    }
    return 0;
}
    )", 2, 1024);

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
    return 0;
}