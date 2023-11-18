#ifndef JUDGE_NOTIFY_H
#define JUDGE_NOTIFY_H

#include "judge_aws.h"
#include "isolate_judge.h"

#include <aws/sns/SNSClient.h>
#include <aws/sns/model/PublishRequest.h>
#include <aws/sns/model/PublishResult.h>

// duplication id를 생성하는 함수: 시간을 기반으로 생성
std::string generate_dup_id() {
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y%m%d%H%M%S");
    return ss.str();
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

    doc.AddMember("id", sub.sumbit_id, allocator);
    doc.AddMember("user_id", sub.user_id, allocator);
    doc.AddMember("problem_id", sub.problem_id, allocator);
    doc.AddMember("language_code", sub.lang, allocator);
    doc.AddMember("memory_limited", sub.max_mem, allocator);
    doc.AddMember("time_limited", sub.max_wtime, allocator);
    doc.AddMember("judge_result", res.res, allocator);
    doc.AddMember("error_message", val_errmsg, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

bool publishToTopic(const Aws::String& message, const Aws::Client::ClientConfiguration &clientConfig) {
    Aws::SNS::SNSClient mySNS(Aws::Auth::AWSCredentials(ACCESS_KEY, SECRET_KEY), clientConfig);
    Aws::SNS::Model::PublishRequest request;
    request.SetMessage(message);
    request.SetTopicArn(TOPIC_ARN);
    request.SetMessageGroupId("judge_result");
    request.SetMessageDeduplicationId(generate_dup_id());

    const Aws::SNS::Model::PublishOutcome outcome = mySNS.Publish(request);
    if(outcome.IsSuccess()) {
        std::cout << "Message published successfully with id " << outcome.GetResult().GetMessageId() << "\n";
        return true;
    } else {
        std::cerr << "Failed to publish message: " << outcome.GetError().GetMessage() << "\n";
        return false;
    }
}

#endif