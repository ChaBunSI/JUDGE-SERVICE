#ifndef PM_CRUD_H
#define PM_CRUD_H

#include <vector>
#include "judge_aws.h"

// 문제 관리 서비스에서 테케 CRUD 요청이 오면, 요청 처리

enum event_type {
    TestCase_NULL, TestCase_ADD, TestCase_UPDATE, TestCase_DELETE
};

event_type get_event_type(const std::string &event_type_str) {
    if (event_type_str == "TestCase_ADD") return TestCase_ADD;
    else if (event_type_str == "TestCase_UPDATE") return TestCase_UPDATE;
    else if (event_type_str == "TestCase_DELETE") return TestCase_DELETE;
    else return TestCase_NULL;
}

using testcase = std::pair<std::string, std::string>; // intput, output

struct crud_request {
    int problem_id;
    std::vector<testcase> testcases;
    event_type type;    
};

// CRUD 요청을 받는 큐에서 메시지를 꺼내는 함수


#endif