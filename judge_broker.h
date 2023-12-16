#ifndef JUDGE_BROKER_H
#define JUDGE_BROKER_H

#include "judge_aws.h"
#include "judge_worker.h"

#include <aws/core/Aws.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/SendMessageResult.h>

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

// duplication id를 생성하는 함수: 시간을 기반으로 생성
std::string generate_dup_id() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_time_t);
    std::stringstream ss;
    ss << std::put_time(now_tm, "%Y%m%d%H%M%S");
    return ss.str();
}

// JudgeTask.fifo에서 메세지를 받아온 후, CPP인지 확인 하는 함수
bool receiveMessageJudgeTask(Aws::String& messageBody, const Aws::Client::ClientConfiguration& clientConfig, Aws::String& messageReceiptHandle, bool &is_cpp) {
    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::ReceiveMessageRequest request;
    request.SetQueueUrl(JT_QUEUE_URL);
    request.SetMaxNumberOfMessages(1);

    const Aws::SQS::Model::ReceiveMessageOutcome outcome = mySQS.ReceiveMessage(request);
    if (outcome.IsSuccess()) {
        const Aws::Vector<Aws::SQS::Model::Message>& messages = outcome.GetResult().GetMessages();
        if(!messages.empty()) {
            messageBody = messages[0].GetBody();
            messageReceiptHandle = messages[0].GetReceiptHandle();
            std::cout << "Received Message From JudgeTask: \n" << messageBody << "\n";

            // 여기서 제출 정보 중 언어 정보만 가져와 C/C++인지 아닌지 확인한다.
            rapidjson::Document doc;
            doc.Parse(messageBody.c_str());
            std::string bd = doc["Message"].GetString();
            doc.Parse(bd.c_str());
            int lang_code = doc["language_code"].GetUint();
            is_cpp = lang_code == C || lang_code == CPP ? true : false;
        }
        else {
            std::cerr << "No message in JudgeTask queue\n";
            return false;
        }
    }
    else {
        std::cerr << "Failed to receive message from JudgeTask queue: " << outcome.GetError().GetMessage() << "\n";
    }
    return outcome.IsSuccess();
}

bool deleteMessageJudgeTask(Aws::String &messageReceiptHandle, const Aws::Client::ClientConfiguration &clientConfig)
{
    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::DeleteMessageRequest request;
    request.SetQueueUrl(JT_QUEUE_URL);
    request.SetReceiptHandle(messageReceiptHandle);

    const Aws::SQS::Model::DeleteMessageOutcome outcome = mySQS.DeleteMessage(request);
    if (outcome.IsSuccess())
    {
        std::cout << "Message deleted successfully.\n";
    }
    else
    {
        std::cout << "Error deleting message from queue: " << outcome.GetError().GetMessage() << "\n";
    }
    return outcome.IsSuccess();
}

// 제출이 C/C++이면 JudgeCPP로 전달
// 다른 프로그래밍 언어라면 JudgeNotCPP로 전달하는 함수
bool deliverMessage(Aws::String &messageBody, const Aws::Client::ClientConfiguration &clientConfig, bool is_cpp) 
{
    const Aws::String queueName = is_cpp ? CPP_QUEUE_NAME : NotCPP_QUEUE_NAME;
    const Aws::String queueUrl = is_cpp ? CPP_QUEUE_URL : NotCPP_QUEUE_URL;
    const Aws::String groupId = is_cpp ? "cppSub" : "notcppSub";

    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::SendMessageRequest request;
    request.SetQueueUrl(queueUrl);
    request.SetMessageBody(messageBody);
    //request.SetMessageGroupId(groupId);
    //request.SetMessageDeduplicationId(generate_dup_id());

    const Aws::SQS::Model::SendMessageOutcome outcome = mySQS.SendMessage(request);
    if (outcome.IsSuccess())
    {
        std::cout << "Message delivered successfully to queue: " << queueName << "\n";
    }
    else
    {
        std::cout << "Error delivering message to queue: " << queueName << " " << outcome.GetError().GetMessage() << "\n";
    }
    return outcome.IsSuccess();
}

#endif