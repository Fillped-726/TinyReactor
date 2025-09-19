#include "thread_pool.hpp"
namespace dts{
ThreadPool::ThreadPool(size_t thread_num)
    : stop_(false), task_count_(0), completed_count_(0) {
    for (size_t i = 0; i < thread_num; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    condition_.wait(lock, [this] {
                        return stop_ || !tasks_.empty();
                    });
                    if (stop_ && tasks_.empty()) {
                        return;
                    }
                    task = std::move(tasks_.front());
                    tasks_.pop();                 
                }
                try {
                    task();
                    ++completed_count_;
                } catch (const std::exception& e) {
                    //log:SPDLOG.error("Task failed: {}", e.what());
                    ++completed_count_;
                } catch (...) {
                    //log:SPDLOG.error("Unknown exception in task");
                    ++completed_count_;
                }
                if (completed_count_ == task_count_ && tasks_.empty()) {
                    condition_.notify_all();   // 只有“全部干完”才唤醒 join_all()
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    if (!workers_.empty()) {
        stop();  // 设置停止标志位
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();  // 等待线程完成
            }
        }
        workers_.clear();  // 清空线程容器
    }
}

void ThreadPool::stop() noexcept {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();  // 唤醒所有等待线程
}

int ThreadPool::get_task_count() const noexcept {
    return task_count_.load(std::memory_order_relaxed);
}

int ThreadPool::get_completed_count() const noexcept {
    return completed_count_.load(std::memory_order_relaxed);
}

void ThreadPool::join_all() noexcept {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    // 等待队列空且任务全部完成
    condition_.wait(lock, [this] {
        return tasks_.empty() && (task_count_ == completed_count_);
    });
}

}//namespace dts;