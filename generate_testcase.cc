#include <bits/stdc++.h>

using namespace std;

random_device rd;
mt19937 gen(rd());

void generate_a_plus_b(int problem_id, int tc_cnt) {
    string problem_dir = "testcases/" + to_string(problem_id);
    uniform_int_distribution<int> dis(1, 100);

    if(!filesystem::exists(problem_dir)) {
        cerr << problem_dir << " does not exist\n";
        return;
    }

    // generate input and output files for "35" problem
    for(int i = 1; i <= tc_cnt; i++) {
        string tc_num = std::to_string(i);
        string input_file = problem_dir + "/" + tc_num + ".in";
        string output_file = problem_dir + "/" + tc_num + ".out";

        ofstream input_fout(input_file);
        ofstream output_fout(output_file);

        int a = dis(gen), b = dis(gen);
        input_fout << a << " " << b << "\n";
        output_fout << a + b << "\n";

        input_fout.close();
        output_fout.close();
    }
}

void generate_sort(int problem_id, int tc_cnt) {
    string problem_dir = "testcases/" + to_string(problem_id);
    uniform_int_distribution<int> dis(1, 1000000);

    if(!filesystem::exists(problem_dir)) {
        cerr << problem_dir << " does not exist\n";
        return;
    }

    // generate input and output files for "35" problem
    for(int i = 1; i <= tc_cnt; i++) {
        string tc_num = std::to_string(i);
        string input_file = problem_dir + "/" + tc_num + ".in";
        string output_file = problem_dir + "/" + tc_num + ".out";

        ofstream input_fout(input_file);
        ofstream output_fout(output_file);

        int n = dis(gen);
        input_fout << n << "\n";
        vector<int> v(n);
        for(int i = 0; i < n; i++) {
            v[i] = dis(gen);
            input_fout << v[i] << "\n";
        }
        input_fout << "\n";
        sort(v.begin(), v.end());
        for(int i = 0; i < n; i++) {
            output_fout << v[i] << "\n";
        }
        output_fout << "\n";

        input_fout.close();
        output_fout.close();
    }
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        cerr << "Usage: ./generate_testcase [problem_id] [tc_cnt]\n";
        return -1;
    }

    int problem_id = atoi(argv[1]);
    int tc_cnt = atoi(argv[2]);
    string problem_dir = "testcases/" + to_string(problem_id);
    
    switch(problem_id) {
        case 21:
            generate_a_plus_b(problem_id, tc_cnt);
            break;
        case 22:
            generate_sort(problem_id, tc_cnt);
            break;
    }

    return 0;
}
