cmake_minimum_required(VERSION 3.20)
project(Common LANGUAGES CXX)

# 查找 nlohmann/json
find_package(nlohmann_json REQUIRED)

# 添加源文件
add_library(common
    src/logger.cpp
    src/config.cpp
    src/utils.cpp
    src/exceptions.cpp
    src/thread_pool.cpp
)

# 指定头文件路径
target_include_directories(common PUBLIC include)

# 链接依赖
target_link_libraries(common PUBLIC nlohmann_json::nlohmann_json)

# 设置 C++ 标准
target_compile_features(common PUBLIC cxx_std_20)