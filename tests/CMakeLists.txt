cmake_minimum_required(VERSION 3.20)
project(CommonTests LANGUAGES CXX)

# ---------- 任务序列化 ----------
add_executable(task_test unit/common-test/task_test.cpp)
target_link_libraries(task_test PRIVATE
    common
    nlohmann_json::nlohmann_json
    GTest::gtest
    GTest::gtest_main
)
target_compile_features(task_test PUBLIC cxx_std_20)
add_test(NAME TaskSerializationTest COMMAND task_test)


# ---------- 线程池测试 ----------
add_executable(thread_pool_test unit/common-test/thread_pool_test.cpp)
target_link_libraries(thread_pool_test PRIVATE
    common                
    GTest::gtest_main
)
target_compile_features(thread_pool_test PUBLIC cxx_std_20)
add_test(NAME ThreadPoolTest COMMAND thread_pool_test)

# ---------- 任务调度器测试 ----------
add_executable(task_executor_test unit/worker-test/task_executor_test.cpp)
target_link_libraries(task_executor_test PRIVATE
    task_executor
    common
    nlohmann_json::nlohmann_json
    Boost::system
    GTest::gtest
    GTest::gtest_main
)
target_compile_features(task_executor_test PUBLIC cxx_std_20)
add_test(NAME TaskExecutorTest COMMAND task_executor_test)

# ---------- gRPC API-Server 单元测试 ----------
add_executable(api_server_test
    unit/api-server-test/api_server_test.cpp
)
# 2. proto 文件已经编进 common_core，直接链接即可
target_link_libraries(api_server_test PRIVATE
    common_core      # 包含 task.grpc.pb.cc / task.pb.cc 的 object
    api_server_lib
    GTest::gtest_main
    grpc++
    glog::glog
    pthread
)
target_compile_features(api_server_test PUBLIC cxx_std_20)
add_test(NAME ApiServerTest COMMAND api_server_test)


