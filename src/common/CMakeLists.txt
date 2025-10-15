cmake_minimum_required(VERSION 3.20)
project(Common LANGUAGES CXX)

# ---------- 路径 ----------
set(PROTO_FILE       "${CMAKE_CURRENT_SOURCE_DIR}/proto/task.proto")
set(PROTO_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_BINARY_DIR})

# ---------- 生成文件 ----------
set(PROTO_SRCS
    "${PROTO_BINARY_DIR}/task.pb.cc"
    "${PROTO_BINARY_DIR}/task.grpc.pb.cc"
)
set(PROTO_HDRS
    "${PROTO_BINARY_DIR}/task.pb.h"
    "${PROTO_BINARY_DIR}/task.grpc.pb.h"
)


# ---------- 生成规则 ----------
set(PROTO_IMPORT_DIRS
    "${PROTO_DIR}"
    "${grpc_SOURCE_DIR}/third_party/protobuf/src"
)
set(_PROTO_PATH_ARGS)
foreach(dir ${PROTO_IMPORT_DIRS})
    list(APPEND _PROTO_PATH_ARGS --proto_path "${dir}")
endforeach()

add_custom_command(
    OUTPUT  ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND ${PROTOC_EXEC}
    ARGS    ${_PROTO_PATH_ARGS}
            --cpp_out          "${PROTO_BINARY_DIR}"
            --grpc_out         "${PROTO_BINARY_DIR}"
            --plugin=protoc-gen-grpc=${GRPC_PLUGIN}
            "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
    COMMENT "Generating gRPC/Protobuf sources"
    VERBATIM
)

# ---------- 库 ----------
add_library(common_core STATIC
    src/logger.cpp
    src/config.cpp
    src/utils.cpp
    src/exceptions.cpp
    src/thread_pool.cpp
    src/grpc_client.cpp
    ${PROTO_SRCS}
)

# 关键：PIC + 导出头文件 + 三方 include
set_target_properties(common_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(common_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROTO_BINARY_DIR}>
        $<BUILD_INTERFACE:${grpc_SOURCE_DIR}/third_party/protobuf/src>
        $<BUILD_INTERFACE:${grpc_SOURCE_DIR}/third_party/abseil-cpp>
    # 安装头文件放到 common/ 子目录，避免冲突
        $<INSTALL_INTERFACE:include/common>
)

target_link_libraries(common_core
    PUBLIC
        Boost::system
        Boost::filesystem
        nlohmann_json::nlohmann_json
        glog::glog
    PRIVATE
        protobuf::libprotobuf
        grpc++
)

# ---------- 接口库（只导出头文件 + 依赖） ----------
add_library(common INTERFACE)
target_link_libraries(common INTERFACE common_core)
target_include_directories(common INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROTO_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/common>
)

# ---------- install ----------
# 把静态库 + 头文件 + 导出文件一起装
install(TARGETS common_core common
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        # 关键：不写 EXPORT 参数！
)

install(DIRECTORY include/
        DESTINATION include/common
        FILES_MATCHING PATTERN "*.hpp"
)
install(FILES ${PROTO_HDRS}
        DESTINATION include/common/proto
)
