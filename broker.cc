#include <iostream>
#include <filesystem>
#include <unistd.h>

#include "judge_aws.h"
#include "problem_manage_crud.h"
#define DEBUG

int main()
{
    std::string tc_dir = "../testcases";
    bool received_crud = false, received_judge = false;
    bool deleted_crud = false, deleted_judge = false;

    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        Aws::String queueName = CRUD_QUEUE_NAME;
        Aws::String queueUrl = CRUD_QUEUE_URL;

        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = Aws::Region::AP_NORTHEAST_2;

        while (1)
        {
            Aws::String crudReceiptHandle;
            crud_request c_req;
            received_crud = receiveMessageCRUD(c_req, clientConfig, crudReceiptHandle);

            if (!received_crud)
            {
                sleep(5);
            }
            else
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
        }
    }
    Aws::ShutdownAPI(options);
    std::cout << "Broker terminates.\n";
    return 0;
}