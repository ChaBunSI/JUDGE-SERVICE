cmake_minimum_required(VERSION 3.0.0)
project(JUDGE-SERVICE)

set(SERVICE_COMPONENTS core sqs sns)
set(CMAKE_CXX_STANDARD 17)

find_package(AWSSDK REQUIRED COMPONENTS ${SERVICE_COMPONENTS})

add_executable(${PROJECT_NAME} worker.cc judge_worker.h judge_aws.h judge_task.h judge_notify.h problem_manage_crud.h)

target_link_libraries(${PROJECT_NAME} ${AWSSDK_LINK_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)