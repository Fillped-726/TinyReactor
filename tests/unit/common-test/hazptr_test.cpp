#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include "hazard_pointer.hpp"

using namespace hazptr;

// ---------- 公共工具：异步等待布尔条件 ----------
template<class Pred>
bool wait_for(Pred&& pred,
              std::chrono::milliseconds timeout = std::chrono::milliseconds(200))
{
    using namespace std::chrono;
    auto end = steady_clock::now() + timeout;
    while (!pred()) {
        if (steady_clock::now() > end) return false;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        HazPtrDomain<int>::defaultDomain().Scan();   // 主动帮忙扫描
    }
    return true;
}

// ==========================
//  1. 基本功能：保护期间不被回收
// ==========================
TEST(HazPtrTest, BasicProtection) {
    int* p = new int(42);
    std::atomic<bool> reclaimed{false};

    auto deleter = [&reclaimed](int* ptr) {
        reclaimed.store(true, std::memory_order_release);
        delete ptr;
    };

    std::thread protector([&p]() {
        HazPtrHolder<int> hp;
        hp.protect(p);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    });

    std::thread reclaimer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        RetirePointer(p, std::function<void(int*)>(deleter));
        // 等待回收完成或超时
        EXPECT_TRUE(wait_for([&] { return reclaimed.load(); }));
    });

    protector.join();
    reclaimer.join();
}

// ==========================
//  2. 多线程压力：随机保护/回收
// ==========================
TEST(HazPtrTest, Stress) {
    constexpr int kNodes = 10'000;
    std::vector<int*> nodes;
    for (int i = 0; i < kNodes; ++i) nodes.push_back(new int(i));

    std::atomic<int> reclaim_count{0};
    auto deleter = [&reclaim_count](int* p) {
        ++reclaim_count;
        delete p;
    };

    auto worker = [&](int id) {
        HazPtrHolder<int> hp;
        for (size_t i = id; i < nodes.size(); i += 4) {
            hp.protect(nodes[i]);
            RetirePointer(nodes[i], std::function<void(int*)>(deleter));
            if (i % 256 == 0) HazPtrDomain<int>::defaultDomain().Scan();
        }
    };

    std::vector<std::thread> ths;
    for (int i = 0; i < 4; ++i) ths.emplace_back(worker, i);
    for (auto& t : ths) t.join();

    // 等待全部回收或超时
    EXPECT_TRUE(wait_for([&] { return reclaim_count.load() == kNodes; }));
}

// ==========================
//  3. 移动语义 RAII 检查
// ==========================
TEST(HazPtrTest, MoveSemantics) {
    int* p = new int(123);
    HazPtrHolder<int> hp1;
    hp1.protect(p);

    HazPtrHolder<int> hp2(std::move(hp1));
    EXPECT_EQ(hp1.get(), nullptr);          // 源失效
    EXPECT_EQ(hp2.get(), p);                // 目标接管

    hp2.release();
    RetirePointer(p);
    HazPtrDomain<int>::defaultDomain().Scan();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    hazptr::HazPtrDomain<int>::RegisterThread(); // 关键
    return RUN_ALL_TESTS();
}