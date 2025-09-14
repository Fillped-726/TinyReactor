// task.hpp
#pragma once
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace dts {

// 任务状态机
enum class TaskState : std::uint8_t {
    PENDING   = 0,   // 刚提交
    RUNNING   = 1,   // 已分发给 Worker
    SUCCESS   = 2,   // 执行成功
    FAILED    = 3,   // 执行失败（可重试）
    TIMEOUT   = 4,   // 超时
    CANCELLED = 5
};

// 资源需求描述（后期调度用）
struct Resource {
    double cpu_core = 0;   // 需要几核
    std::uint64_t mem_mb = 0; // 需要多少 MB
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Resource, cpu_core, mem_mb)
};

// 任务分片信息（大任务拆片）
struct Shard {
    std::uint32_t shard_id = 0;
    std::uint32_t total_shards = 1;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Shard, shard_id, total_shards)
};

// 主任务结构体
struct Task {
    std::string task_id;            // UUID，全局唯一
    std::string client_id;          // 谁提交的
    std::uint32_t priority = 0;     // 越大越优先
    TaskState state = TaskState::PENDING;

    /* 函数描述 */
    std::string func_name;          // 函数名（如 "fib" / "md5" / "map_reduce"）
    nlohmann::json func_params;     // 任意 JSON 参数列表

    Resource required;              // 资源需求
    Shard shard;                    // 分片信息（整包任务 total_shards=1）

    /* 超时与重试 */
    std::uint32_t timeout_ms = 30'000; // 默认 30 s
    std::uint32_t max_retry = 3;
    std::uint32_t retry_count = 0;

    /* 时间戳（调度器填写） */
    std::int64_t submit_ts = 0;     // 提交时间（unix epoch ms）
    std::int64_t start_ts  = 0;     // 开始执行时间
    std::int64_t finish_ts = 0;     // 结束时间

    /* 结果回填 */
    nlohmann::json result;          // 执行结果（成功/失败都写）
    std::string error_msg;          // 若失败，写原因

    // JSON 序列化/反序列化
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Task,
        task_id, client_id, priority, state,
        func_name, func_params,
        required, shard,
        timeout_ms, max_retry, retry_count,
        submit_ts, start_ts, finish_ts,
        result, error_msg)
};

} // namespace dts