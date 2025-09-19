#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <boost/asio.hpp>
#include "task.hpp"

namespace dts {

class TaskExecutor {
public:
    using TaskFunction = std::function<nlohmann::json(const nlohmann::json& params,std::shared_ptr<Task>)>;

    TaskExecutor(boost::asio::io_context& io_context);
    ~TaskExecutor() = default;

    // 注册任务处理函数
    void register_function(const std::string& func_name, TaskFunction func);

    // 执行任务（异步）
    void execute_task(std::shared_ptr<Task> task);

private:
    // 实际执行任务的逻辑
    void run_task(std::shared_ptr<Task> task);

    // 检查资源需求是否满足
    bool check_resources(const Resource& required);

    //检查错误类型
    static bool is_retryable_error(const boost::system::error_code& ec);

    // 更新任务状态
    void update_task_state(std::shared_ptr<Task> task, TaskState state,
                          const nlohmann::json& result = {}, const std::string& error_msg = "");

    boost::asio::io_context& io_context_;  // 用于异步执行
    std::unordered_map<std::string, TaskFunction> functions_;  // 函数映射
    // 假设系统资源（可通过 resource-reporter 获取）
    Resource available_resources_ = {4.0, 8192};  // 示例：4核，8GB

    inline static std::atomic<int> retrying_cnt{0};
    static constexpr int MAX_CONCURRENT_RETRY = 10;
};

} // namespace dts