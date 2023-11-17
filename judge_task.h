#ifndef JUDGE_TASK_H
#define JUDGE_TASK_H

#include <iostream>
#include <aws/core/Aws.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/CreateQueueRequest.h>
#include <aws/sqs/model/GetQueueUrlRequest.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/SendMessageResult.h>

#include "isolate_judge.h"
#include "judge_sqs.h"

// Message의 Body 부분:
/* "id", "user_id", "is_passed", "is_judged", "judge_status", "source", "language_code",
 * "created_time", "start_time", "end_time", // 형식은 "YYYY-MM-DDTHH:MM:SS.00000+09:00"
 * "memory_limited", "time_limited"
 */

user_submission unmarshal(const Aws::SQS::Model::Message& msg) {
    user_submission res;
    std::string body = msg.GetBody();

    std::cout << "Received Message: \n";
    std::cout << body << "\n";
    // TODO: body 파싱
    int pos = 0;

    return res;
}

/* 채점 큐에서 제출 정보를 꺼내는 부분 */
/* TODO:
   * 1. SQS에서 메세지 받아오기
   * 2. 메세지의 body 부분 파싱하여 user_submission 구조체로 변환
   *    2-1. body 전체가 한 문자열이라, 파싱해야 함
   *    2-2. body 파싱 후, 코드 부분 url_decode 필요
   * 3. user_submission 구조체 반환
*/

bool receiveMessage(user_submission &sub, const Aws::Client::ClientConfiguration &clientConfig) {
    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::ReceiveMessageRequest request;
    request.SetQueueUrl(QUEUE_URL);
    request.SetMaxNumberOfMessages(1);

    const auto outcome = mySQS.ReceiveMessage(request);
    if(outcome.IsSuccess()) {
        const auto &messages = outcome.GetResult().GetMessages();
        if(!messages.empty()) {
            const Aws::SQS::Model::Message &message = messages[0];
            sub = unmarshal(message);
        } else {
            std::cerr << "No messages received from queue\n";
        }
    } else {
        std::cerr << "Error receiving message from queue: " << outcome.GetError().GetMessage() << "\n";
    }
    return outcome.IsSuccess();
}

// 채점 큐에서 메세지를 잘 받는지 확인하는 함수
bool sendMessageSelf(const Aws::Client::ClientConfiguration &clientConfig) {
    Aws::String msg_body = "{\"id\": 68834166, \"problem_id\": 30455, \"user_id\": pridom1118, \"is_passed\": 1, \"is_judged\": 0, \"judge_status\": 301, \"source\": %23include%20%3Cbits%2Fstdc%2B%2B.h%3E%0A%23define%20fastio%20cin.tie%280%29-%3Esync_with_stdio%280%29%0A%0Ausing%20namespace%20std%3B%0A%0Aint%20main%28%29%20%7B%0A%20%20%20%20fastio%3B%0A%20%20%20%20int%20N%3B%20cin%20%3E%3E%20N%3B%0A%20%20%20%20if%28N%20%26%201%29%20cout%20%3C%3C%20%22Goose%22%3B%0A%20%20%20%20else%20cout%20%3C%3C%20%22Duck%22%3B%0A%20%20%20%20return%200%3B%0A%7D, \"language_code\": CPP, \"created_time\": 1, \"start_time\": 1, \"end_time\": 1, \"memory_limited\": 128, \"time_limited\": 1000}";
    Aws::SQS::SQSClient mySQS(CREDENTIALS, clientConfig);
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