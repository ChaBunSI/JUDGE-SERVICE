#ifndef JUDGE_NOTIFY_H
#define JUDGE_NOTIFY_H

#include "judge_aws.h"
#include "isolate_judge.h"

// 채점 결과 SNS Topic으로 발신용
#include <aws/sns/SNSClient.h>
#include <aws/sns/model/PublishRequest.h>
#include <aws/sns/model/PublishResult.h>

// 실시간 제출 현황
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/SendMessageRequest.h>

// duplication id를 생성하는 함수: 시간을 기반으로 생성
std::string generate_dup_id(int submit_id, int tc_id=-1) {
    std::string res = std::to_string(submit_id);
    return tc_id == -1 ? res : res + "_" + std::to_string(tc_id);   
}

Aws::String judge_res_to_aws_string(judge_info &res, user_submission &sub) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    rapidjson::Value val_errmsg(rapidjson::kStringType);

    if(res.err_msg.size() == 0) {
        val_errmsg.SetNull();
    } else {
        val_errmsg.SetString(res.err_msg.c_str(), res.err_msg.size(), allocator);
    }

    doc.AddMember("id", sub.submit_id, allocator);
    doc.AddMember("user_id", sub.user_id, allocator);
    doc.AddMember("problem_id", sub.problem_id, allocator);
    doc.AddMember("language_code", sub.lang, allocator);
    doc.AddMember("memory_limited", sub.max_mem, allocator);
    doc.AddMember("time_limited", sub.max_time, allocator);
    doc.AddMember("judge_result", res.res, allocator);
    doc.AddMember("error_message", val_errmsg, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

bool publishToTopic(int submit_id, const Aws::String& message, const Aws::Client::ClientConfiguration &clientConfig) {
    Aws::SNS::SNSClient mySNS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SNS::Model::PublishRequest request;
    request.SetMessage(message);
    request.SetTopicArn(TOPIC_ARN);
    request.SetMessageGroupId("judge_result");
    request.SetMessageDeduplicationId(generate_dup_id(submit_id));

    const Aws::SNS::Model::PublishOutcome outcome = mySNS.Publish(request);
    if(outcome.IsSuccess()) {
        std::cout << "Message published successfully with id " << outcome.GetResult().GetMessageId() << "\n";
        return true;
    } else {
        std::cerr << "Failed to publish message: " << outcome.GetError().GetMessage() << "\n";
        return false;
    }
}

// 실시간 채점 현황 관련 함수들
// SNS Topic으로 채점 결과를 보낼 때와 달리,
// SQS Queue로 채점 결과를 보낸다. (여러 서비스들에게 정보를 보내는 것이 아니라 1:1로 보내는 것이기 때문)

Aws::String cur_res_to_aws_string(int submit_id, int problem_id, int tc_cnt, int tc_cur, judge_result res) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("id", submit_id, allocator);
    doc.AddMember("problem_id", problem_id, allocator);
    doc.AddMember("tc_total", tc_cnt, allocator);
    doc.AddMember("tc_cur", tc_cur, allocator);
    doc.AddMember("result", res, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

bool sendToQueue(int submit_id, int tc_id, const Aws::String& message, const Aws::Client::ClientConfiguration &clientConfig) {
    Aws::SQS::SQSClient mySQS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SQS::Model::SendMessageRequest request;

    request.SetQueueUrl(RT_QUEUE_URL);
    request.SetMessageGroupId("rt_judge_result");
    request.SetMessageDeduplicationId(generate_dup_id(submit_id, tc_id));
    request.SetMessageBody(message);

    const Aws::SQS::Model::SendMessageOutcome outcome = mySQS.SendMessage(request);

    if(outcome.IsSuccess()) {
        std::cout << "Message published successfully with id " << outcome.GetResult().GetMessageId() << "\n";
        return true;
    } else {
        std::cerr << "Failed to publish message: " << outcome.GetError().GetMessage() << "\n";
        return false;
    }
}

#endif