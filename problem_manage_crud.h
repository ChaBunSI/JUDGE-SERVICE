#ifndef PM_CRUD_H
#define PM_CRUD_H

#include <vector>
#include <fstream>

#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/ReceiveMessageResult.h>
#include <aws/sqs/model/DeleteMessageRequest.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "judge_aws.h"

// 문제 관리 서비스에서 테케 CRUD 요청이 오면, 요청 처리
enum event_type
{
    TestCase_NULL,
    TestCase_ADD,
    TestCase_UPDATE,
    TestCase_DELETE
};

event_type get_event_type(const std::string &event_type_str)
{
    if (event_type_str == "TestCase_ADD")
        return TestCase_ADD;
    else if (event_type_str == "TestCase_UPDATE")
        return TestCase_UPDATE;
    else if (event_type_str == "TestCase_DELETE")
        return TestCase_DELETE;
    else
        return TestCase_NULL;
}

struct testcase
{
    size_t id;
    std::string input;
    std::string output;

    testcase(int _id, const std::string &input, const std::string &output) : id(_id), input(input), output(output) {}
    testcase() : id(0), input(""), output("") {}
};

struct crud_request
{
    int problem_id;
    std::vector<testcase> testcases;
    event_type type;
};

crud_request unmarshalCRUD(const Aws::SQS::Model::Message &msg)
{
    std::string body = msg.GetBody();
    rapidjson::Document doc;
    doc.Parse(body.c_str());
    body = doc["Message"].GetString();
    std::cout << "Received Message: \n"
              << body << "\n";
    doc.Parse(body.c_str());
    crud_request c_req;

    c_req.problem_id = doc["problemId"].GetInt();
    rapidjson::Value &testcases = doc["testCases"];
    c_req.type = get_event_type(doc["eventType"].GetString());

    for (rapidjson::SizeType i = 0; i < testcases.Size(); i++)
    {
        rapidjson::Value &tc = testcases[i];
        c_req.testcases.emplace_back(tc["id"].GetInt(), tc["input"].GetString(), tc["output"].GetString());
    }
    return c_req;
}

// CRUD 요청을 받는 큐에서 메시지를 꺼내는 함수
bool receiveMessageCRUD(crud_request &c_req, const Aws::Client::ClientConfiguration &clientConfig, Aws::String &messageReceiptHandle)
{
    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::ReceiveMessageRequest request;
    request.SetQueueUrl(CRUD_QUEUE_URL);
    request.SetMaxNumberOfMessages(1);

    const Aws::SQS::Model::ReceiveMessageOutcome outcome = mySQS.ReceiveMessage(request);
    if (outcome.IsSuccess())
    {
        const Aws::Vector<Aws::SQS::Model::Message> &messages = outcome.GetResult().GetMessages();
        if (!messages.empty())
        {
            const Aws::SQS::Model::Message &message = messages[0];
            c_req = unmarshalCRUD(message);
            messageReceiptHandle = message.GetReceiptHandle();
        }
        else
        {
            std::cerr << "No messages received from queue\n";
            return false;
        }
    }
    else
    {
        std::cerr << "Failed to receive the message from queue: " << outcome.GetError().GetMessage() << "\n";
    }
    return outcome.IsSuccess();
}

bool deleteMessageCRUD(Aws::String &messageReceiptHandle, const Aws::Client::ClientConfiguration &clientConfig)
{
    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::DeleteMessageRequest request;
    request.SetQueueUrl(CRUD_QUEUE_URL);
    request.SetReceiptHandle(messageReceiptHandle);

    const Aws::SQS::Model::DeleteMessageOutcome outcome = mySQS.DeleteMessage(request);
    if (outcome.IsSuccess())
    {
        std::cout << "Message deleted successfully\n";
        return true;
    }
    else
    {
        std::cerr << "Error deleting message: " << outcome.GetError().GetMessage() << "\n";
        return false;
    }
}

bool process_crud_add(crud_request &c_req)
{
    std::string tc_dir = "../testcases/" + std::to_string(c_req.problem_id) + "/";
    std::filesystem::path tc_path(tc_dir);
    std::cout << "ADD Request!\n";

    // 새로운 문제인 경우, 해당 디렉토리 생성
    if (!std::filesystem::exists(tc_path))
    {
        std::cout << "Created a new directory for the problem " << c_req.problem_id << "\n";
        bool create_dir_res = std::filesystem::create_directory(tc_path);
        if (!create_dir_res)
        {
            std::cerr << "Failed to create a new directory for the problem " << c_req.problem_id << "\n";
            return false;
        }
    }

    for (auto &tc : c_req.testcases)
    {
        std::string tc_num = tc.id < 10 ? "0" + std::to_string(tc.id) : std::to_string(tc.id);
        std::ofstream tc_in = std::ofstream(tc_dir + tc_num + ".in", std::ios::out | std::ios::trunc);
        std::ofstream tc_out = std::ofstream(tc_dir + tc_num + ".out", std::ios::out | std::ios::trunc);

        tc_in << tc.input;
        tc_out << tc.output;

        tc_in.close();
        tc_out.close();
    }
    return true;
}

bool process_crud_update(crud_request &c_req)
{
    std::cout << "UPDATE Request!\n";
    std::string tc_dir = "../testcases/" + std::to_string(c_req.problem_id) + "/";
    std::filesystem::path tc_path(tc_dir);
    bool res = false;

    // 문제가 존재하지 않는 경우
    if (!std::filesystem::exists(tc_path))
    {
        std::cerr << "Cannot update the problem " << c_req.problem_id << " -- it does not exist.\n";
        return false;
    }

    for (auto &tc : c_req.testcases)
    {
        std::string tc_num = std::to_string(tc.id);

        // 테스트케이스가 존재하는 경우에만 update
        // 존재하는 id에만 한해 update 진행, 나머지는 **무시**
        std::filesystem::path tc_in_path(tc_dir + tc_num + ".in"), tc_out_path(tc_dir + tc_num + ".out");

        if (std::filesystem::exists(tc_in_path))
        {
            std::ofstream tc_in = std::ofstream(tc_dir + tc_num + ".in", std::ios::out | std::ios::trunc);
            tc_in << tc.input;
            tc_in.close();
            res = true;
        }
        else
        {
            std::cerr << "Cannot update the testcase " << tc_num << ".in -- it does not exist.\n";
        }

        if (std::filesystem::exists(tc_out_path))
        {
            std::ofstream tc_out = std::ofstream(tc_dir + tc_num + ".out", std::ios::out | std::ios::trunc);
            tc_out << tc.output;
            tc_out.close();
            res = true;
        }
        else
        {
            std::cerr << "Cannot update the testcase " << tc_num << ".out -- it does not exist.\n";
        }
    }
    return res;
}

bool process_crud_delete(crud_request &c_req)
{
    std::string tc_dir = "../testcases/" + std::to_string(c_req.problem_id) + "/";
    std::filesystem::path tc_path(tc_dir);
    bool res = false;

    // 문제가 존재하지 않는 경우
    if (!std::filesystem::exists(tc_path))
    {
        std::cerr << "Cannot delete the problem " << c_req.problem_id << " -- it does not exist.\n";
        return false;
    }

    std::cout << "DELETE Request!\n";

    for (auto &tc : c_req.testcases)
    {
        std::string tc_num = std::to_string(tc.id);
        std::filesystem::path tc_in_path(tc_dir + tc_num + ".in"), tc_out_path(tc_dir + tc_num + ".out");

        // 테스트 케이스가 존재하지 않는 경우
        if (std::filesystem::exists(tc_in_path))
        {
            std::filesystem::remove(tc_dir + tc_num + ".in");
            res = true;
        }
        else
        {
            std::cerr << "Cannot delete the testcase " << tc_num << ".in -- it does not exist.\n";
        }

        if (std::filesystem::exists(tc_out_path))
        {
            std::filesystem::remove(tc_dir + tc_num + ".out");
            res = true;
        }
        else
        {
            std::cerr << "Cannot delete the testcase " << tc_num << ".out -- it does not exist.\n";
        }
    }
    return res;
}

bool process_crud_request(crud_request &c_req)
{
    bool res = false;
    switch (c_req.type)
    {
    case TestCase_ADD:
        res = process_crud_add(c_req);
        break;
    case TestCase_UPDATE:
        res = process_crud_update(c_req);
        break;
    case TestCase_DELETE:
        res = process_crud_delete(c_req);
        break;
    default:
        std::cerr << "Invalid event type\n";
        break;
    }
    return res;
}

#endif