cmake_minimum_required(VERSION 3.20)
project(TaskExecutor LANGUAGES CXX)

# 添加库
add_library(task_executor
    src/task_executor.cpp
    src/task_runner.cpp
    src/result_handler.cpp
)

# 指定头文件路径
target_include_directories(task_executor PUBLIC include)

# 链接依赖
target_link_libraries(task_executor PUBLIC
    common
    Boost::system
    nlohmann_json::nlohmann_json
)

# 设置 C++ 标准
target_compile_features(task_executor PUBLIC cxx_std_20)