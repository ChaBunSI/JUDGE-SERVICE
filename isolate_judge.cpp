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

int main() {
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
    

    return 0;
}