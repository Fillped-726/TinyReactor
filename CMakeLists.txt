cmake_minimum_required(VERSION 3.20)
project(DistributedTaskSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost 1.80 REQUIRED COMPONENTS system filesystem)
find_package(nlohmann_json REQUIRED)

# ---------- 依赖 ----------
include(FetchContent)
FetchContent_Declare(
    hiredis
    GIT_REPOSITORY https://github.com/redis/hiredis.git
    GIT_TAG        v1.2.0
)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(hiredis googletest)

# ---------- 子目录 ----------
add_subdirectory(src/common)
# add_subdirectory(third_party/quill)
# add_subdirectory(third_party/cpp-httplib)
# 其他业务子目录随开随用

# ---------- 示例可执行 ----------
add_executable(api_server src/api-server/src/main.cpp)
target_link_libraries(api_server
    PRIVATE
        common
        Boost::system
        hiredis::hiredis
        nlohmann_json::nlohmann_json
        # quill
        # cpp-httplib
)

# ---------- 测试 ----------
enable_testing()
add_subdirectory(tests)

# ---------- 安装 ----------
install(TARGETS api_server DESTINATION bin)