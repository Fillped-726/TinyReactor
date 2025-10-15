#pragma once
#include <chrono>
#include "task.pb.h"
#include "task.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dts {

using PbTask      = ::dts::proto::Task;
using PbTaskState = ::dts::proto::TaskState;

// 返回当前时间戳（毫秒）
inline uint64_t get_current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// 辅助：nlohmann::json ↔ google::protobuf::Struct
void JsonToStruct(const json& j, google::protobuf::Struct* proto);
json StructToJson(const google::protobuf::Struct& proto);

// Task 类型转换
PbTask TaskToProto(const Task& task);
Task TaskFromProto(const PbTask& proto);

} // namespace dts