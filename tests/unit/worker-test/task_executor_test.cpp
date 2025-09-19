#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include "task_executor.hpp"
#include "task.hpp"
#include "utils.hpp"

namespace dts {

class TaskExecutorTest : public ::testing::Test {
protected:
    boost::asio::io_context io_context_;
    std::unique_ptr<TaskExecutor> executor_;

    void SetUp() override {
        executor_ = std::make_unique<TaskExecutor>(io_context_);
    }
};

TEST_F(TaskExecutorTest, ExecuteFibTask) {
    auto task = std::make_shared<Task>();
    task->task_id = "uuid-1234";
    task->client_id = "client-001";
    task->priority = 5;
    task->state = TaskState::PENDING;
    task->func_name = "fib";
    task->func_params = {{"n", 10}};
    task->required = {1.0, 512};
    task->submit_ts = get_current_timestamp_ms()-100;

    executor_->execute_task(task);

    // 运行 io_context 直到任务完成
    io_context_.run();

    // 验证结果
    EXPECT_EQ(task->state, TaskState::SUCCESS);
    EXPECT_EQ(task->result["result"], 55);  // fib(10) = 55
    EXPECT_TRUE(task->error_msg.empty());
    EXPECT_GT(task->finish_ts, task->submit_ts);
}

TEST_F(TaskExecutorTest, UnknownFunction) {
    auto task = std::make_shared<Task>();
    task->task_id = "uuid-1235";
    task->func_name = "unknown";
    task->required = {1.0, 512};
    task->submit_ts = get_current_timestamp_ms();

    executor_->execute_task(task);
    io_context_.run();

    EXPECT_EQ(task->state, TaskState::FAILED);
    EXPECT_TRUE(task->result.empty());
    EXPECT_EQ(task->error_msg, "Unknown function: unknown");
}

TEST_F(TaskExecutorTest, InsufficientResources) {
    auto task = std::make_shared<Task>();
    task->task_id = "uuid-1236";
    task->func_name = "fib";
    task->func_params = {{"n", 5}};
    task->required = {10.0, 16384};  // 超过可用资源（4核，8GB）
    task->submit_ts = get_current_timestamp_ms();

    executor_->execute_task(task);
    io_context_.run();

    EXPECT_EQ(task->state, TaskState::FAILED);
    EXPECT_TRUE(task->result.empty());
    EXPECT_EQ(task->error_msg, "Insufficient resources");
}

TEST_F(TaskExecutorTest, Timeout) {
    auto task = std::make_shared<Task>();
    task->task_id = "uuid-1237";
    task->func_name = "fib";
    task->func_params = {{"n", 5}};
    task->required = {1.0, 512};
    task->timeout_ms = 1000;  // 1秒
    task->submit_ts = get_current_timestamp_ms() - 2000;  // 模拟已超时

    executor_->execute_task(task);
    io_context_.run();

    EXPECT_EQ(task->state, TaskState::TIMEOUT);
    EXPECT_TRUE(task->result.empty());
    EXPECT_EQ(task->error_msg, "Task timed out");
}

} // namespace dts

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}