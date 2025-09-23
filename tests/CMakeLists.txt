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
