#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <iomanip>
#include <chrono>

enum judge_result {
    NJ, AC, WA, CE, RE, TLE, MLE, OLE, PE //NJ: Not Judged
};

struct judge_info {
    size_t tc_id;
    size_t time, mem;
    judge_result res;
};

void print_statistics(std::vector<judge_info>& judge_res) {
    int len = judge_res.size();
    double wtime_sum = 0.0, mem_sum = 0.0, max_wtime = 0.0, max_mem = 0.0;
    int ac_cnt = 0, not_ac_cnt = 0;

    for(auto& e: judge_res) {
        if(e.res == AC) {
            wtime_sum += e.time;
            mem_sum += e.mem;
            max_wtime = std::max(max_wtime, (double)e.time);
            ac_cnt++;
        }
        else not_ac_cnt++;
    }

    std::cout << "30461 Statistics: \n";
    std::cout << "AC: " << ac_cnt << "\n";
    std::cout << "WA: " << not_ac_cnt << "\n";
    std::cout << "Average time: " << std::setprecision(5) << wtime_sum / ac_cnt << "ms\n";
    std::cout << "Average mem: " << std::setprecision(5) << mem_sum / ac_cnt << "\n";
    std::cout << "Max time: " << std::setprecision(5) << max_wtime << "ms\n";
}

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

int main() {
    std::string problem_id = "30461";
    std::string cur_tc_dir = "./" + problem_id;
    auto cur_tc_dir_iter = std::filesystem::directory_iterator(cur_tc_dir);

    int cnt_tc = std::count_if(
        begin(cur_tc_dir_iter), end(cur_tc_dir_iter),
        [](const std::filesystem::directory_entry& e) {
            return e.is_regular_file();
        }
    ) / 2;

    /* Compiles the file from the judge queue, catches any Compile errors */
    int compile_res = system("g++ Main.cc -o Main -O2 -Wall -lm -static -std=gnu++20 -DONLINE_JUDGE -DBOJ");
    if(compile_res != 0) {
        std::cout << "Compile Error\n";
        return 0;
    }
    
    std::vector<judge_info> judge_res(cnt_tc);
    
    for(int i = 1; i <= cnt_tc; i++) {
        judge_info& cur_judge_info = judge_res[i-1];
        std::string tc_num = i < 10 ? "0" + std::to_string(i) : std::to_string(i);
        std::ifstream tc_out("./" + problem_id + "/" + tc_num + ".out");

        // Initialize the judge info
        cur_judge_info.tc_id = i;
        cur_judge_info.res = NJ;
        cur_judge_info.time = 0.0;
        cur_judge_info.mem = 0.0;

        // Run the program and measure the time and memory
        std::string cur_cmd = "./Main < " + problem_id + "/" + tc_num + ".in > " + "usr_" + tc_num + ".out";

        auto start = std::chrono::high_resolution_clock::now();
        system(cur_cmd.c_str());
        auto end = std::chrono::high_resolution_clock::now();
        
        // Check if the user's output is correct
        std::ifstream usr_out("usr_" + tc_num + ".out");
        std::string tc_out_str, usr_out_str;
        
        while(std::getline(tc_out, tc_out_str) && std::getline(usr_out, usr_out_str)) {
            if(!strip_and_compare(tc_out_str, usr_out_str)) {
                cur_judge_info.res = WA;
                std::cout << "WA on TC " << i << ", expected: " << tc_out_str << ", got: " << usr_out_str << "\n";
                break;
            } else cur_judge_info.res = AC;
        }

        cur_judge_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    print_statistics(judge_res);
    system("rm Main usr_*.out");
    return 0;
}