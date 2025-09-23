#include "thread_pool.hpp"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <vector>
#include <random>
#include <thread>
#include <iostream>

namespace dts::test {

/* ---------- 工具：等待原子变量达到值 ---------- */
template<typename T>
bool wait_eq(const std::atomic<T>& a, T v,
             std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
{
    auto end = std::chrono::steady_clock::now() + timeout;
    while (a.load(std::memory_order_relaxed) != v &&
           std::chrono::steady_clock::now() < end)
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    return a.load(std::memory_order_relaxed) == v;
}

/* ================================================================
 * 功能测试
 * ================================================================ */

/* 1. 基本执行 */
TEST(ThreadPool, BasicExecution)
{
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    const int kTasks = 10'000;
    for (int i = 0; i < kTasks; ++i)
        pool.enqueue([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    ASSERT_TRUE(wait_eq(counter, kTasks));
    EXPECT_EQ(counter.load(), kTasks);
}

/* 2. 队列满边界：把唯一线程堵死，再塞到满 */
TEST(ThreadPool, QueueFull)
{
    constexpr size_t kCap = 64;
    ThreadPool pool(1, kCap);                 // 单线程
    std::binary_semaphore blocker{0};         // @@ 堵线程
    std::atomic<bool> pop_ok{false};

    /* 先塞一个长任务占住 worker */
    pool.enqueue([&blocker] { blocker.acquire(); });

    /* 再塞 kCap 个任务，队列必满 */
    std::atomic<int> produced{0};
    for (size_t i = 0; i < kCap; ++i)
        pool.enqueue([&produced] { produced.fetch_add(1); });  // 不会丢

    /* 再塞一个，此次 enqueue 必须等队列有空槽才能返回 */
    std::thread t([&pool, &pop_ok] {
        pool.enqueue([&pop_ok] { pop_ok.store(true); });
    });

    /* 等一会确保 t 已经在自旋 */
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    /* 释放 blocker，让 worker 继续 */
    blocker.release();
    t.join();                                   // 必须成功返回
    ASSERT_TRUE(wait_eq(pop_ok, true));
    EXPECT_EQ(produced.load(), static_cast<int>(kCap));
}

/* 3. 动态线程伸缩 */
TEST(ThreadPool, DynamicThreadAdjust)
{
   ThreadPool pool(2);
    std::atomic<size_t> cnt{};
    for (int i = 0; i < 50; ++i) {          // 50 次采样
        cnt.store(pool.get_thread_count());
        std::cout << "[DBG] active = " << cnt.load() << '\n';
        if (cnt == 2) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ASSERT_TRUE(cnt == 2); 

    pool.add_threads(3);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_TRUE(wait_eq(std::atomic<size_t>{pool.get_thread_count()}, static_cast<size_t>(5),
                        std::chrono::seconds(1)));

    pool.remove_threads(4);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(wait_eq(std::atomic<size_t>{pool.get_thread_count()}, static_cast<size_t>(1),
                        std::chrono::seconds(1)));
}

/* 4. 停止后抛异常 */
TEST(ThreadPool, StopThenReject)
{
    ThreadPool pool(1);
    pool.shutdown();                 // 手动停止
    EXPECT_THROW(pool.enqueue([]{}), std::runtime_error);
}

/* 5. 任务抛异常不杀死线程 */
TEST(ThreadPool, ExceptionSurvive)
{
    ThreadPool pool(1);
    std::atomic<int> exc_cnt{0};
    /* 扔一个会抛异常的任务 */
    pool.enqueue([&exc_cnt] {
        try { throw 42; }
        catch (...) { exc_cnt.fetch_add(1); }
    });
    ASSERT_TRUE(wait_eq(exc_cnt, 1));

    /* 再扔一个正常任务，若线程已死则永远完不成 */
    std::atomic<bool> done{false};
    pool.enqueue([&done] { done.store(true); });
    ASSERT_TRUE(wait_eq(done, true));
}

/* 6. 多生产者并发入队 */
TEST(ThreadPool, MultiProducer)
{
    const int kProducers = 8;
    const int kTasksPerProd = 10'000;
    ThreadPool pool(4);
    std::atomic<int> sum{0};

    std::vector<std::thread> producers;
    producers.reserve(kProducers);
    for (int p = 0; p < kProducers; ++p)
        producers.emplace_back([&pool, &sum] {
            for (int i = 0; i < kTasksPerProd; ++i)
                pool.enqueue([&sum] { sum.fetch_add(1, std::memory_order_relaxed); });
        });
    for (auto& t : producers) t.join();
    ASSERT_TRUE(wait_eq(sum, kProducers * kTasksPerProd));
}

/* 7. 严格 FIFO 顺序（单线程池保证） */
TEST(ThreadPool, StrictFIFO)
{
    ThreadPool pool(1);          // 单 worker
    std::vector<int> seq(5);     // 0..4
    std::atomic<int> idx{0};
    for (int i = 0; i < 5; ++i)
        pool.enqueue([&, i] { seq[idx.fetch_add(1)] = i; });
    ASSERT_TRUE(wait_eq(idx, 5));
    for (int i = 0; i < 5; ++i) EXPECT_EQ(seq[i], i);
}

/* ================================================================
 * 性能基准
 * ================================================================ */

/* 8. 吞吐量 */
TEST(ThreadPool, PerfThroughput)
{
    ThreadPool pool(std::thread::hardware_concurrency());
    const int kTasks = 1'000'000;
    std::atomic<int> done{0};
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kTasks; ++i)
        pool.enqueue([&done] { done.fetch_add(1, std::memory_order_relaxed); });
    ASSERT_TRUE(wait_eq(done, kTasks, std::chrono::seconds(10)));
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "[ PERF ] throughput " << kTasks << " tasks  "
              << ms << " ms  "
              << (kTasks * 1.0 / ms) << " kops\n";
}

/* 9. 平均延迟 */
TEST(ThreadPool, PerfLatency)
{
    ThreadPool pool(std::thread::hardware_concurrency());
    const int kSamples = 50'000;
    std::atomic<int> left{kSamples};
    std::vector<uint64_t> lat_us(kSamples);
    std::mutex lat_mu;
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<> dist(1, 10); // 1–10 µs 随机工作

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kSamples; ++i) {
        auto submit = std::chrono::steady_clock::now();
        pool.enqueue([&, i, submit] {
            std::this_thread::sleep_for(std::chrono::microseconds(dist(rng)));
            auto finish = std::chrono::steady_clock::now();
            uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(
                              finish - submit).count();
            {
                std::lock_guard<std::mutex> lock(lat_mu);
                lat_us[i] = us;
            }
            left.fetch_sub(1);
        });
    }
    ASSERT_TRUE(wait_eq(left, 0, std::chrono::seconds(30)));
    uint64_t sum = 0;
    for (uint64_t v : lat_us) sum += v;
    std::cout << "[ PERF ] average latency "
              << (sum / kSamples) << " µs\n";
}

/* 10. 突发流量 */
TEST(ThreadPool, PerfBurst)
{
    ThreadPool pool(std::thread::hardware_concurrency());
    const int kBurst = 100'000;
    std::atomic<int> done{0};
    auto t0 = std::chrono::steady_clock::now();
    /* 0.1 s 内灌完 */
    for (int i = 0; i < kBurst; ++i)
        pool.enqueue([&done] { done.fetch_add(1, std::memory_order_relaxed); });
    ASSERT_TRUE(wait_eq(done, kBurst, std::chrono::seconds(5)));
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "[ PERF ] burst " << kBurst << " tasks in "
              << ms << " ms\n";
}

}   // namespace dts::test