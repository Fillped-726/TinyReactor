cmake_minimum_required(VERSION 3.20)
project(CommonTests LANGUAGES CXX)

# 添加测试可执行文件
add_executable(task_test unit/common-test/task_test.cpp)

# 链接依赖
target_link_libraries(task_test PRIVATE
    common
    nlohmann_json::nlohmann_json
    GTest::gtest
    GTest::gtest_main
)

# 设置 C++ 标准
target_compile_features(task_test PUBLIC cxx_std_20)

# 注册测试到 CTest
add_test(NAME TaskSerializationTest COMMAND task_test)