#include "task_executor.hpp"
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <boost/asio/post.hpp>
#include<boost/asio/steady_timer.hpp>
#include "utils.hpp"  // 假设包含 get_current_timestamp_ms

namespace dts {

TaskExecutor::TaskExecutor(boost::asio::io_context& io_context)
    : io_context_(io_context) {
    // 示例：注册内置函数
    register_function("fib", [](const nlohmann::json& params,std::shared_ptr<Task> t) -> nlohmann::json {
        int n = params.value("n", 0);
        if (n < 0) throw std::runtime_error("Negative input for fib");
        if (n <= 1) return {{"result", n}};
        int a = 0, b = 1;
        for (int i = 2; i <= n; ++i) {
            if (t->cancelled->load()) return nlohmann::json{{"result", "cancelled"}};
            int c = a + b;
            a = b;
            b = c;
        }
        return {{"result", b}};
    });
}

void TaskExecutor::register_function(const std::string& func_name, TaskFunction func) {
    functions_[func_name] = std::move(func);
}

void TaskExecutor::execute_task(std::shared_ptr<Task> task) {
    // 异步提交任务
    boost::asio::post(io_context_, [this, task]() {
        run_task(task);
    });
}

void TaskExecutor::run_task(std::shared_ptr<Task> task) {

        // 检查资源
        if (!check_resources(task->required)) {
            update_task_state(task, TaskState::FAILED, {}, "Insufficient resources");
            return;
        }

        // 检查超时
        auto now_ms = get_current_timestamp_ms();  // 假设 utils.hpp 提供
        if (task->timeout_ms > 0 && (now_ms - task->submit_ts) > task->timeout_ms) {
            update_task_state(task, TaskState::TIMEOUT, {}, "Task timed out");
            return;
        }

        // 2. 动态计算执行超时（关键！）
        uint32_t exec_timeout = task->timeout_ms - (now_ms - task->submit_ts);
        
        // 3. 创建异步超时定时器
        auto exec_timer = std::make_shared<boost::asio::steady_timer>(io_context_);
        exec_timer->expires_after(std::chrono::milliseconds(exec_timeout));
        
        // 4. 超时回调：设置cancelled标志
        exec_timer->async_wait([this, task, exec_timer](boost::system::error_code ec) {
            if (!ec) { // 超时发生
                task->cancelled->store(true);
                update_task_state(task, TaskState::TIMEOUT, {}, "Execution timeout");
            }
        });

    try {

        // 设置开始时间
        task->start_ts = now_ms;
        task->state = TaskState::RUNNING;

        // 查找函数
        auto it = functions_.find(task->func_name);
        if (it == functions_.end()) {
            update_task_state(task, TaskState::FAILED, {}, "Unknown function: " + task->func_name);
            exec_timer->cancel();
            return;
        }

        // 执行函数
        nlohmann::json result = it->second(task->func_params, task);
        update_task_state(task, TaskState::SUCCESS, result);
        exec_timer->cancel();
        
    } catch (const boost::system::system_error& e) {
        // 异常处理
        if (is_retryable_error(e.code()) && task->retry_count < task->max_retry) {
            if (retrying_cnt.fetch_add(1) >= MAX_CONCURRENT_RETRY) {
                retrying_cnt--;                 // 令牌用完，直接失败
                update_task_state(task, TaskState::FAILED, {}, "Retry quota full");
                return;
            }
            //延迟重试
            auto timeout = std::chrono::seconds(1 << std::min(task->retry_count, 4u));
            auto timer = std::make_shared<boost::asio::steady_timer>(io_context_, timeout);
            timer->async_wait([this, task, timer, exec_timer](boost::system::error_code) {
                retrying_cnt--;
                execute_task(task);
            });
            task->retry_count++;
        } else {
            exec_timer->cancel();
            update_task_state(task, TaskState::FAILED, {}, "Execution failed: " + std::string(e.what()));
        }
    }
}

bool TaskExecutor::check_resources(const Resource& required) {
    // 简单检查（实际应与 resource-reporter 集成）
    return required.cpu_core <= available_resources_.cpu_core &&
           required.mem_mb <= available_resources_.mem_mb;
}

void TaskExecutor::update_task_state(std::shared_ptr<Task> task, TaskState state, const nlohmann::json& result, const std::string& error_msg) {
    task->state = state;
    task->result = result;
    task->error_msg = error_msg;
    task->finish_ts = get_current_timestamp_ms();
}

bool TaskExecutor::is_retryable_error(const boost::system::error_code& ec) {
    // ✅ 明确列出所有可重试错误
    return ec == boost::asio::error::timed_out ||
           ec == boost::asio::error::connection_refused ||
           ec == boost::asio::error::host_unreachable;
}

} // namespace dts