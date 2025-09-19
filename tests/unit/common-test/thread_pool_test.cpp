#include <gtest/gtest.h>
#include <atomic>
#include "thread_pool.hpp"
using dts::ThreadPool;

/* ---------- 全局命名空间 ---------- */

TEST(ThreadPoolTest, SingleTaskExecution) {
    ThreadPool pool(2);
    int result = 0;
    auto f = pool.enqueue([&result] { result = 42; });
    f.wait();                       // 等任务真跑完
    EXPECT_EQ(result, 42);
}

TEST(ThreadPoolTest, Concurrency) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    for (int i = 0; i < 1000; ++i)
        pool.enqueue([&counter] { counter++; });
    pool.join_all();
    EXPECT_EQ(counter.load(), 1000);
}

TEST(ThreadPoolTest, ExceptionHandling) {
    ThreadPool pool(1);
    auto fut = pool.enqueue([] { throw std::runtime_error("err"); });
    EXPECT_THROW(fut.get(), std::runtime_error);   // 异常会重抛
}

TEST(ThreadPoolTest, StopBehavior) {
    ThreadPool pool(2);
    pool.stop();
    pool.join_all();
    EXPECT_EQ(pool.get_task_count(), 0);
}