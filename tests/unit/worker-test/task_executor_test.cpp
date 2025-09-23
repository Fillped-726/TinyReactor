/********************************************************************
 *  test_task_executor.cpp
 *  修正版：枚举值与 TaskState 定义完全一致
 *******************************************************************/
#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "task_executor.hpp"
#include "task.hpp"
#include "utils.hpp"
#include <boost/asio/error.hpp>
#include <atomic>
#include <thread>
#include <boost/asio/executor_work_guard.hpp>

using namespace dts;
using json = nlohmann::json;

namespace { constexpr int MAX_CONCURRENT_RETRY = 10; }

/* ---------- 辅助 ---------- */
static std::shared_ptr<Task> make_task(std::string func,
                                       json params,
                                       uint64_t timeout_ms = 0,
                                       Resource req = {1, 1024},
                                       uint32_t max_retry = 0)
{
    auto t         = std::make_shared<Task>();
    t->func_name   = std::move(func);
    t->func_params = std::move(params);
    t->timeout_ms  = timeout_ms;
    t->required    = req;
    t->max_retry   = max_retry;
    t->submit_ts   = get_current_timestamp_ms();  
    return t;
}

static void wait_done(const std::shared_ptr<Task>& t)
{
    while (t->state == TaskState::PENDING || t->state == TaskState::RUNNING)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

/* ---------- 测试套件 ---------- */
class TaskExecutorTest : public ::testing::Test
{
protected:
    boost::asio::io_context io;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_;
    std::unique_ptr<TaskExecutor> exe;
    std::thread io_th;

    void SetUp() override
    {
        work_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
                    boost::asio::make_work_guard(io));
        exe   = std::make_unique<TaskExecutor>(io);
        io_th = std::thread([this] { io.run(); });
    }

    void TearDown() override
    {
        work_.reset();          // 先撤销 work guard，让 run() 能自然退出
        io.stop();              // 再停 io_context
        io_th.join();           // 最后等线程结束
    }
};

/* ---------------- 7 条用例 ---------------- */

TEST_F(TaskExecutorTest, RegisterAndSuccess)
{
    exe->register_function("add", [](const json& p, std::shared_ptr<Task>) {
        int a = p.value("a", 0), b = p.value("b", 0);
        return json{{"result", a + b}};
    });
    auto t = make_task("add", json{{"a", 3}, {"b", 4}},100);
    exe->execute_task(t);
    wait_done(t);
    EXPECT_EQ(t->state, TaskState::SUCCESS);
    EXPECT_EQ(t->result["result"], 7);
}

TEST_F(TaskExecutorTest, InsufficientResource)
{
    auto t = make_task("add", json{}, 0, {99, 999999});
    exe->execute_task(t);
    wait_done(t);
    EXPECT_EQ(t->state, TaskState::FAILED);
}

TEST_F(TaskExecutorTest, OverallTimeout)
{
    // 可中断的 sleep：10 ms 切片
    exe->register_function("sleep", [](const json&, std::shared_ptr<Task> task) {
        for (int i = 0; i < 30; ++i) {          // 30 × 10 ms = 300 ms
            if (task->cancelled->load()) return json{{"result", "cancelled"}};
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return json{{"result", "ok"}};
    });

    auto t = make_task("sleep", json{}, 100);
    exe->execute_task(t);
    wait_done(t);
    EXPECT_EQ(t->state, TaskState::TIMEOUT);   // 现在一定返回 TIMEOUT
}

TEST_F(TaskExecutorTest, CancelDuringExecution)
{
    exe->register_function("long_fib", [](const json&, std::shared_ptr<Task> task) {
        for (int i = 0; i < 35; ++i) {
            if (task->cancelled->load()) return json{{"result", "cancelled"}};
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return json{{"result", "done"}};
    });
    auto t = make_task("long_fib", json{});
    exe->execute_task(t);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    t->cancelled->store(true);
    wait_done(t);
    EXPECT_EQ(t->state, TaskState::SUCCESS);
    EXPECT_EQ(t->result["result"].get<std::string>(), "cancelled");
}

TEST_F(TaskExecutorTest, RetryEventuallySucceed)
{
    std::atomic<int> calls{0};
    exe->register_function("maybe_fail", [&calls](const json&, std::shared_ptr<Task>) -> json {
        if (++calls < 3)
            throw boost::system::system_error(boost::asio::error::connection_refused);
        return json{{"result", "ok"}};
    });
    auto t = make_task("maybe_fail", json{}, 10000, {1, 1024}, 5);
    exe->execute_task(t);
    wait_done(t);
    EXPECT_EQ(t->state, TaskState::SUCCESS);
    EXPECT_EQ(t->retry_count, 2);
}

TEST_F(TaskExecutorTest, RetryQuotaExhausted)
{
    exe->register_function("always_fail", [](const json&, std::shared_ptr<Task>) -> json {
        throw boost::system::system_error(boost::asio::error::host_unreachable);
    });
    constexpr int N = MAX_CONCURRENT_RETRY + 2;
    std::vector<std::shared_ptr<Task>> tasks;
    for (int i = 0; i < N; ++i) {
        tasks.push_back(make_task("always_fail", json{}, 10000, {1, 1024}, 1));
        exe->execute_task(tasks.back());
    }
    for (auto& t : tasks) wait_done(t);
    int quota_full = 0;
    for (auto& t : tasks)
        if (t->error_msg.find("Retry quota full") != std::string::npos) ++quota_full;
    EXPECT_GE(quota_full, 2);
}

TEST_F(TaskExecutorTest, UnknownFunction)
{
    auto t = make_task("no_such_func", json{},100);
    exe->execute_task(t);
    wait_done(t);
    EXPECT_EQ(t->state, TaskState::FAILED);
}