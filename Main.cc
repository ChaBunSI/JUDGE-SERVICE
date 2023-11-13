#include <bits/stdc++.h>
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