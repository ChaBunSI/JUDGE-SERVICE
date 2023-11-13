#include <map>

enum judge_result {
    NJ, AC, WA, CE, RE, TLE, MLE, OLE, PE //NJ: Not Judged
};

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
    size_t max_wtime, max_mem, code_bytes; // in ms, MB, bytes
    judge_result res;
    std::string err_msg; // 에러 메세지 출력용

    judge_info() { max_wtime = max_mem = code_bytes = 0; res = NJ; err_msg = ""; }
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
// echo code | tee Main.${ext}라는 명령어를 사용해서 파일을 만들 것이다.
// 이때 ext는 위 lang_configs에서 정의한 ext를 사용할 것이다.
struct user_submission {
    size_t sumbit_id, problem_id;
    language lang;
    std::string user_id;
    std::string code;
    size_t max_wtime, max_mem, code_bytes;

    user_submission(size_t si, size_t pi, language l, std::string uid, std::string c, size_t mw, size_t mm): sumbit_id(si), problem_id(pi), lang(l), user_id(uid), code(c), max_wtime(mw * 1000), max_mem(mm) { code_bytes = code.size(); }
    user_submission() {
        sumbit_id = 0U;
        lang = CPP;
        user_id = "";
        code = "";
        max_wtime = max_mem = code_bytes = 0;
    }

    error_code compile_code(lang_config& cur_config) {
        std::string compile_cmd;
        std::string mkfile_cmd = "echo " + code + " | tee Main" + cur_config.ext;
        int mkfile_res = system(mkfile_cmd.c_str());

        // 파일 만들기 실패
        if(mkfile_res != 0) {
            std::cerr << "Failed to make the file\n";
            return FILE_CREATE_ERROR;
        }

        // 컴파일 에러 -> 컴파일 에러 시 user_sumbission
        int compile_res = system(cur_config.compile_cmd.c_str());
        if(compile_res != 0) {
            std::cerr << "Compile Error\n";
            return COMPILE_ERROR;
        }
        return NO_ERROR;
    }
};