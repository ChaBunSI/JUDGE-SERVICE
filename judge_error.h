#ifndef JUDGE_ERROR_H
#define JUDGE_ERROR_H

#include <string>

// SIGNALS for x86/ARM
// 샌드박스 내에서 에러가 발생할 때, 에러 발생 당시의 시그널들을 뜻함
// ex) Caught fatal signal 11 -> SIGSEGV

const std::string SIGNALS[32] = {
    "", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT",
    "SIGIOT", "SIGBUS", "SIGKILL", "SIGUSR1", "SIGSEGV",
    "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD",
    "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG",
    "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO",
    "SIGPWR", "SIGSYS"
};

#endif