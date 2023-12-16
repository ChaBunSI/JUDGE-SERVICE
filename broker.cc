#include <iostream>
#include <filesystem>
#include <unistd.h>

#include "problem_manage_crud.h"
#include "judge_broker.h"
#define DEBUG

int main()
{
    std::string tc_dir = "../testcases";
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        Aws::String queueName = CRUD_QUEUE_NAME;
        Aws::String queueUrl = CRUD_QUEUE_URL;

        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = Aws::Region::AP_NORTHEAST_2;

        while (1)
        {
            bool received_crud = false, received_sub = false;
            bool deleted_crud = false, deleted_sub = false;
            bool is_delivered = false;

            Aws::String crudReceiptHandle;
            crud_request c_req;
            received_crud = receiveMessageCRUD(c_req, clientConfig, crudReceiptHandle);
        
            if (received_crud)
            {
                std::cout << "Received CRUD message successfully\n";
#ifdef DEBUG
                std::cout << "=======================================\n";
                std::cout << "Problem ID: " << c_req.problem_id << "\n";
                std::cout << "Event Type: " << c_req.type << "\n";
                std::cout << "========================================\n";
#endif
                
                process_crud_request(c_req);
                deleted_crud = deleteMessageCRUD(crudReceiptHandle, clientConfig);

                if (!deleted_crud)
                {
                    std::cerr << "Failed to delete message from CRUD queue\n";
                }
                else
                {
                    std::cout << "Deleted CRUD message successfully\n";
                }
            }

            Aws::String subReceiptHandle;
            Aws::String messageBody;
            bool is_cpp = false;
            received_sub = receiveMessageJudgeTask(messageBody, clientConfig, subReceiptHandle, is_cpp);
            
            if(received_sub) {
                if(is_cpp) {
                    std::cout << "Received C/C++ submission message successfully\n";
                } else {
                    std::cout << "Received Not C/C++ submission message successfully\n";
                }
                is_delivered = deliverMessage(messageBody, clientConfig, is_cpp);

                if(!is_delivered) {
                    std::cerr << "Failed to deliver message to CPP or NotCPP queue\n";
                } else {
                    std::cout << "Delivered message to CPP or NotCPP queue successfully\n";
                    deleteMessageJudgeTask(subReceiptHandle, clientConfig);
                }
            }
        }
    }
    
    Aws::ShutdownAPI(options);
    std::cout << "Broker terminates.\n";
    return 0;
}