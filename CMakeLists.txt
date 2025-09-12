# 主CMake配置文件
# 根目录 CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(DistributedTaskSystem 
    VERSION 1.0.0
    DESCRIPTION "分布式任务调度系统"
    LANGUAGES CXX)

# C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 选项
option(BUILD_TESTS "构建测试" ON)
option(BUILD_EXAMPLES "构建示例" ON)
option(BUILD_SHARED_LIBS "构建共享库" OFF)

# 包含cmake模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 编译器警告
include(CompilerWarnings)

# 标准项目设置
include(StandardProjectSettings)

# 查找依赖
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem thread)

# 第三方库
add_subdirectory(third_party)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/common/include)

# 子目录
add_subdirectory(src/api-server)
add_subdirectory(src/scheduler/node-manager)
add_subdirectory(src/scheduler/task-scheduler)
add_subdirectory(src/scheduler/resource-monitor)
add_subdirectory(src/worker/task-executor)
add_subdirectory(src/worker/resource-reporter)
add_subdirectory(src/storage/redis)
add_subdirectory(src/storage/leveldb)
add_subdirectory(src/common)

# 测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 安装
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY configs/ DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/distributed-task-system)