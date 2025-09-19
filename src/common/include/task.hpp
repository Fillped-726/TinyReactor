#pragma once
#include <nlohmann/json.hpp>
#include <atomic>
#include <memory>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace dts {

enum class TaskState : std::uint8_t {
    PENDING = 0, RUNNING = 1, SUCCESS = 2, FAILED = 3, TIMEOUT = 4, CANCELLED = 5
};

struct Resource {
    double cpu_core = 0;
    std::uint64_t mem_mb = 0;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Resource, cpu_core, mem_mb)
};

struct Shard {
    std::uint32_t shard_id = 0;
    std::uint32_t total_shards = 1;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Shard, shard_id, total_shards)
};

struct Task {
    std::string task_id;
    std::string client_id;
    std::uint32_t priority = 0;
    TaskState state = TaskState::PENDING;

    std::shared_ptr<std::atomic<bool>> cancelled =
        std::make_shared<std::atomic<bool>>(false);

    std::string func_name;
    nlohmann::json func_params;
    Resource required;
    Shard shard;
    std::uint32_t timeout_ms = 30'000;
    std::uint32_t max_retry = 3;
    std::uint32_t retry_count = 0;
    std::int64_t submit_ts = 0;
    std::int64_t start_ts = 0;
    std::int64_t finish_ts = 0;
    nlohmann::json result;
    std::string error_msg;
};

// 手动 JSON 转换，跳过 cancelled
inline void to_json(nlohmann::json& j, const Task& t) {
    j = {
        {"task_id", t.task_id},
        {"client_id", t.client_id},
        {"priority", t.priority},
        {"state", t.state},
        {"func_name", t.func_name},
        {"func_params", t.func_params},
        {"required", t.required},
        {"shard", t.shard},
        {"timeout_ms", t.timeout_ms},
        {"max_retry", t.max_retry},
        {"retry_count", t.retry_count},
        {"submit_ts", t.submit_ts},
        {"start_ts", t.start_ts},
        {"finish_ts", t.finish_ts},
        {"result", t.result},
        {"error_msg", t.error_msg}
    };
}

inline void from_json(const nlohmann::json& j, Task& t) {
    j.at("task_id").get_to(t.task_id);
    j.at("client_id").get_to(t.client_id);
    j.at("priority").get_to(t.priority);
    j.at("state").get_to(t.state);
    j.at("func_name").get_to(t.func_name);
    j.at("func_params").get_to(t.func_params);
    j.at("required").get_to(t.required);
    j.at("shard").get_to(t.shard);
    j.at("timeout_ms").get_to(t.timeout_ms);
    j.at("max_retry").get_to(t.max_retry);
    j.at("retry_count").get_to(t.retry_count);
    j.at("submit_ts").get_to(t.submit_ts);
    j.at("start_ts").get_to(t.start_ts);
    j.at("finish_ts").get_to(t.finish_ts);
    j.at("result").get_to(t.result);
    j.at("error_msg").get_to(t.error_msg);
}

}  // namespace dts