#ifndef ISOLATE_JUDGE_H
#define ISOLATE_JUDGE_H

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <iomanip>
#include <map>
 
/* Judge Result:
 * NJ: Not Judged
 * AC: Accepted
 * WA: Wrong Answer
 * CE: Compile Error
 * RE: Runtime Error
 * TLE: Time Limit Exceeded
 * MLE: Memory Limit Exceeded
 * OLE: Output Limit Exceeded (출력 초과)
 * SE: Sandbox Execution Error 
 */

enum judge_result {
    NJ, AC, WA, CE, RE, TLE, MLE, OLE, SE
};

std::string judge_result_to_string(judge_result res) {
    switch(res) {
        case NJ: return "NJ";
        case AC: return "AC";
        case WA: return "WA";
        case CE: return "CE";
        case RE: return "RE";
        case TLE: return "TLE";
        case MLE: return "MLE";
        case OLE: return "OLE";
        case SE: return "SE";
        default: return "NJ";
    }
}

// 채점 프로세스 내부에서만 사용되는 에러 코드입니다.
enum error_code {
    NO_ERROR, COMPILE_ERROR, RUNTIME_ERROR, TIME_LIMIT_EXCEEDED, FILE_CREATE_ERROR
};

enum language {
    C, CPP, JAVA, PYTHON
};

// 각 언어별 컴파일 옵션, 실행 옵션, 시간 제한, 메모리 제한 등을 저장하는 구조체
struct lang_config {
    language lang;
    std::string ext;
    std::string compile_cmd;
    std::string run_cmd;
    int(*get_max_wtime)(int);
    int(*get_max_mem)(int);

    lang_config(language l, std::string e, std::string cc, std::string rc, int(*gwm)(int), int(*gmm)(int)): lang(l), ext(e), compile_cmd(cc), run_cmd(rc), get_max_wtime(gwm), get_max_mem(gmm) {}
    lang_config() {
        lang = CPP;
        ext = ".cc";
        compile_cmd = "g++ Main.cc -o Main -O2 -Wall -lm -static -std=gnu++20 -DONLINE_JUDGE -DBOJ";
        run_cmd = "./Main";
        get_max_wtime = [](int wtime) -> int { return wtime; };
        get_max_mem = [](int mem) -> int { return mem; };
    }
};

// 각 테스트 케이스별로 시간, 메모리, 결과 등을 저장하는 구조체
// 이 구조체를 기반으로 채점 결과를 알릴 것이다.
struct judge_info {
    size_t wtime, mem, code_bytes; // in ms, MB, bytes
    size_t tc_id;
    judge_result res;
    std::string err_msg; // 에러 메세지 출력용
    judge_info() { wtime = mem = code_bytes = 0; res = NJ; err_msg = ""; }
};

// 각 언어별 컴파일 옵션, 실행 옵션, 시간 제한, 메모리 제한 등을 저장하는 map
// 채점 큐에서 하나씩 꺼내, 해당 언어의 컴파일 옵션, 실행 옵션, 시간 제한, 메모리 제한 등을 가져올 것이다.
std::map<language, lang_config> lang_configs = {
    {C, lang_config(C, ".c", "gcc Main.c -o Main -O2 -Wall -lm -static -std=gnu11 -DONLINE_JUDGE -DBOJ", "./Main", [](int wtime) -> int { return wtime; }, [](int mem) -> int { return mem; })},
    {CPP, lang_config(CPP, ".cc", "g++ Main.cc -o Main -O2 -Wall -lm -static -std=gnu++20 -DONLINE_JUDGE -DBOJ", "./Main", [](int wtime) -> int { return wtime; }, [](int mem) -> int { return mem; })},
    {JAVA, lang_config(JAVA, ".java", "javac -release 11 -J-Xms1024m -J-Xmx1920m -J-Xss512m -encoding UTF-8 Main.java", "java -Xms1024m -Xmx1920m -Xss512m -Dfile.encoding=UTF-8 -XX:+UseSerialGC -DONLINE_JUDGE=1 -DBOJ=1 Main", [](int wtime) -> int { return 2*wtime+1; }, [](int mem) -> int { return 2*mem+16; })},
    {PYTHON, lang_config(PYTHON, ".py",R"(python3 -W ignore Main.py)", "python3 -W ignore Main.py", [](int wtime) -> int { return 3*wtime+2; }, [](int mem) -> int { return 2*mem+32; })}
};

// 채점 큐에서 꺼낸 제출 정보
// 이때 코드 자체가 긴 문자열로 되어있기 때문에, 문자열을 기반으로 파일을 만드는 작업 필요
// 사용하는 언어를 기반으로 파일을 만들고 컴파일할 것이다.
// 파일 입출력을 사용해 주어진 코드 (문자열)을 파일로 작성할 것이다.
// 이때 확장자 (ext)는 위 lang_configs에서 정의한 확장자를 사용할 것이다.
struct user_submission {
    size_t sumbit_id, problem_id, user_id;
    language lang;
    std::string code;
    size_t max_wtime, max_mem, code_bytes; // ms, MB, bytes

    user_submission(size_t si, size_t pi, size_t ui, language l, std::string c, size_t mw, size_t mm): sumbit_id(si), problem_id(pi), lang(l), user_id(ui), code(c), max_wtime(mw), max_mem(mm) { code_bytes = code.size(); }
    user_submission() {
        sumbit_id = 0U;
        problem_id = 0U;
        user_id = 0U;
        lang = CPP;
        code = "";
        max_wtime = max_mem = code_bytes = 0;
    }

    error_code compile_code(lang_config& cur_config) {
        std::string compile_cmd;
        std::ofstream cur_code;
        cur_code.open("Main" + cur_config.ext, std::ios::out | std::ios::trunc);

        // 파일 만들기
        if(!cur_code.is_open()) {
            std::cerr << "Failed to make the file\n";
            return FILE_CREATE_ERROR; 
        }

        cur_code << code;
        cur_code.close();

        // 컴파일 에러 -> 컴파일 에러 시 user_sumbission
        int compile_res = system(cur_config.compile_cmd.c_str());
        if(compile_res != 0) {
            std::cerr << "Compile Error\n";
            return COMPILE_ERROR;
        }
        return NO_ERROR;
    }
};

// 두 .out 파일을 비교하는 함수
// 테스트 케이스의 출력 파일과, 사용자의 출력 파일을 비교한다.
bool strip_and_compare(std::string& out, std::string& usr_out) {
    // Strip the string
    out.erase(std::remove(out.begin(), out.end(), ' '), out.end());
    out.erase(std::remove(out.begin(), out.end(), '\n'), out.end());
    out.erase(std::remove(out.begin(), out.end(), '\r'), out.end());
    usr_out.erase(std::remove(usr_out.begin(), usr_out.end(), ' '), usr_out.end());
    usr_out.erase(std::remove(usr_out.begin(), usr_out.end(), '\n'), usr_out.end());
    usr_out.erase(std::remove(usr_out.begin(), usr_out.end(), '\r'), usr_out.end());
    return out == usr_out;
}

bool remove_rawnline_compare(std::string out, std::string usr_out) {
    std::string new_usr_out;
    
    for (int i = 0; i < usr_out.size(); i++) {
        if (usr_out[i] == '\\') {
            if (usr_out[i + 1] == 'n') {
                new_usr_out += '\n';
                i++;
            } else {
                new_usr_out += usr_out[i];
            }
        } else {
            new_usr_out += usr_out[i];
        }
    }
    return out == new_usr_out;
}

// 채점 결과를 출력하는 함수
// TODO: 채점 중 가장 긴 실행 시간과 가장 큰 메모리 사용량을 출력해야 한다.
void print_statistics(std::vector<judge_info>& judge_res, user_submission& cur_sub, judge_info& cur_judge_info) {
    int len = judge_res.size();
    int ac_cnt = 0, not_ac_cnt = 0;

    for(auto& e: judge_res) {
        if(e.res == AC) ac_cnt++;
        else not_ac_cnt++;
    }

    std::cout << std::to_string(cur_sub.problem_id) + " Statistics: \n";
    std::cout << "AC: " << ac_cnt << "\n";
    std::cout << "WA: " << not_ac_cnt << "\n";

    if(judge_res.size() == ac_cnt) cur_judge_info.res = AC;
    else cur_judge_info.res = WA;
}

#endif