#ifndef COMMON_THREAD_POOL_THREAD_POOL_HPP_
#define COMMON_THREAD_POOL_THREAD_POOL_HPP_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace dts {

class ThreadPool {
 public:
  /* 构造与析构 */
  explicit ThreadPool(size_t thread_num);
  ~ThreadPool() noexcept;

  /* 禁止拷贝与移动 */
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  /* 任务提交接口 */
  template <typename F, typename... Args>
  auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

  /* 生命周期管理 */
  void stop() noexcept;
  void join_all() noexcept;

  /* 统计接口 */
  int get_task_count() const noexcept;
  int get_completed_count() const noexcept;

 private:

  /* 成员变量 */
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  mutable std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::atomic<bool> stop_{false};
  std::atomic<int> task_count_{0};
  std::atomic<int> completed_count_{0};
};

/* 模板定义需放在头文件中 */
template <typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<decltype(f(args...))> {
  using RetType = std::invoke_result_t<F, Args...>;

  auto task = std::make_shared<std::packaged_task<RetType()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<RetType> future = task->get_future();

  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (stop_) {
      throw std::runtime_error("enqueue on stopped ThreadPool");
    }
    tasks_.emplace([task]() { (*task)(); });
    ++task_count_;
  }

  condition_.notify_one();
  return future;
}

}  // namespace dts

#endif  // COMMON_THREAD_POOL_THREAD_POOL_HPP_