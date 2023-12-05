#ifndef JUDGE_TASK_H
#define JUDGE_TASK_H

#include <iostream>
#include <iomanip>
#include <sstream>
#include <aws/core/Aws.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/CreateQueueRequest.h>
#include <aws/sqs/model/GetQueueUrlRequest.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/SendMessageResult.h>
#include <aws/sqs/model/DeleteMessageRequest.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "isolate_judge.h"
#include "judge_aws.h"

std::string format_code(const std::string code) {
    std::string new_code;
    new_code.reserve(code.size());
    for (int i = 0; i < code.size(); i++) {
        if (code[i] == '\\') {
            if(!(i+1 < code.size() && code[i+1] == 'n')) new_code += '\\';
        }
        new_code += code[i];
    }
    return new_code;
}

// Message의 Body 부분:
/* "id", "user_id", "is_passed", "is_judged", "judge_status", "source", "language_code",
 * "created_time", "start_time", "end_time", // 형식은 "YYYY-MM-DDTHH:MM:SS.00000+09:00"
 * "memory_limited", "time_limited"
 */

/* 채점 큐에서 제출 정보를 꺼내는 부분 */
user_submission unmarshal(const Aws::SQS::Model::Message& msg) {
    std::string body = msg.GetBody();
    rapidjson::Document doc;
    doc.Parse(body.c_str());
    body = doc["Message"].GetString();
    std::cout << "Received Message: \n" << body << "\n";
    doc.Parse(body.c_str());
    return user_submission(doc["id"].GetUint64(), doc["problem_id"].GetUint64(), doc["user_id"].GetUint64(), static_cast<language>(doc["language_code"].GetUint()), format_code(doc["source"].GetString()), doc["memory_limited"].GetUint(), doc["time_limited"].GetUint());
}

bool receiveMessageFromJudgeTask(user_submission &sub, const Aws::Client::ClientConfiguration &clientConfig, Aws::String &messageReceiptHandle) {
    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::ReceiveMessageRequest request;
    request.SetQueueUrl(QUEUE_URL);
    request.SetMaxNumberOfMessages(1);

    const Aws::SQS::Model::ReceiveMessageOutcome outcome = mySQS.ReceiveMessage(request);
    if(outcome.IsSuccess()) {
        const Aws::Vector<Aws::SQS::Model::Message> &messages = outcome.GetResult().GetMessages();
        if(!messages.empty()) {
            const Aws::SQS::Model::Message &message = messages[0];
            sub = unmarshal(message);
            messageReceiptHandle = message.GetReceiptHandle();
        } else {
            std::cerr << "No messages received from queue\n";
            return false;
        }
    } else {
        std::cerr << "Error receiving message from queue: " << outcome.GetError().GetMessage() << "\n";
    }
    return outcome.IsSuccess();
}

bool deleteMessageJudgeTask(Aws::String &messageReceiptHandle, const Aws::Client::ClientConfiguration &clientConfig) {
    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::DeleteMessageRequest request;
    request.SetQueueUrl(QUEUE_URL);
    request.SetReceiptHandle(messageReceiptHandle);

    const Aws::SQS::Model::DeleteMessageOutcome outcome = mySQS.DeleteMessage(request);
    if(outcome.IsSuccess()) {
        std::cout << "Message deleted successfully.\n";
    } else {
        std::cout << "Error deleting message from queue: " << outcome.GetError().GetMessage() << "\n";
    }
    return outcome.IsSuccess();
}

// 채점 큐에서 메세지를 잘 받는지 확인하는 함수. 오로지 테스트 용입니다.
bool sendMessageSelf(const Aws::Client::ClientConfiguration &clientConfig) {
    Aws::String msg_body = "{\"id\": 68834166, \"problem_id\": 30455, \"user_id\": 990612, \"is_passed\": 1, \"is_judged\": 0, \"judge_status\": 301, \"source\": %23include%20%3Cbits%2Fstdc%2B%2B.h%3E%0A%23define%20fastio%20cin.tie%280%29-%3Esync_with_stdio%280%29%0A%0Ausing%20namespace%20std%3B%0A%0Aint%20main%28%29%20%7B%0A%20%20%20%20fastio%3B%0A%20%20%20%20int%20N%3B%20cin%20%3E%3E%20N%3B%0A%20%20%20%20if%28N%20%26%201%29%20cout%20%3C%3C%20%22Goose%22%3B%0A%20%20%20%20else%20cout%20%3C%3C%20%22Duck%22%3B%0A%20%20%20%20return%200%3B%0A%7D, \"language_code\": 1, \"created_time\": 1, \"start_time\": 1, \"end_time\": 1, \"memory_limited\": 128, \"time_limited\": 1000}";
    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::SendMessageRequest request;
    request.SetQueueUrl(QUEUE_URL);
    request.SetMessageBody(msg_body);

    const Aws::SQS::Model::SendMessageOutcome outcome = mySQS.SendMessage(request);
    if(outcome.IsSuccess()) {
        std::cout << "Message sent successfully.\n";
    } else {
        std::cout << "Error sending message to queue: " << outcome.GetError().GetMessage() << "\n";
    }
    return outcome.IsSuccess();
}

#endif