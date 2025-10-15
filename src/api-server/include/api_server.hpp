#pragma once
#include <grpcpp/alarm.h>
#include <grpcpp/grpcpp.h>
#include <grpc/support/time.h>
#include <vector>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include "logger.hpp" 
#include "task.grpc.pb.h"       
#include "task.pb.h"

namespace dts {

using dts::proto::Task;
using dts::proto::TaskResponse;
using dts::proto::TaskService;
using AsyncTaskService = dts::proto::TaskService::AsyncService;

// 前向声明
class AsyncServer;

// 通用异步上下文（一次写成，终身复用）
template <class Service, class Request, class Response>
class AsyncCallContext {
public:
    using ProceedFunc = std::function<void(AsyncCallContext*)>;

    AsyncCallContext(Service* svc,
                     grpc::ServerCompletionQueue* cq,
                     ProceedFunc pf)
        : service_(svc), cq_(cq), responder_(&ctx_),
          proceed_(std::move(pf)), status_(CallStatus::CREATE) {
        RequestNext();          // 第一次注册
    }

    void Proceed(bool ok = true) {
        if (!ok) {
            delete this;
            return;
        }

        switch (status_) {
        case CallStatus::CREATE:
            if (proceed_) proceed_(this);
            status_ = CallStatus::FINISH;
            responder_.Finish(response_, grpc::Status::OK, this);
            break;

        case CallStatus::FINISH:
            {
                // 1. 保存现场
                auto* svc = service_;
                auto* cq  = cq_;
                auto  pf  = proceed_;
                // 2. 自杀
                delete this;
                // 3. 同线程立即注册新对象（防止 CQ 饿死）
                new AsyncCallContext<Service,Request,Response>(svc, cq, pf);
            }
            return;   // 必须 return，不再访问已销毁内存
        }
    }

    void RequestNext();

    grpc::ServerContext   ctx_;
    Request               request_;
    Response              response_;

private:
    enum class CallStatus { CREATE, FINISH, REARM };
    Service*                                    service_;
    grpc::ServerCompletionQueue*                cq_;
    grpc::ServerAsyncResponseWriter<Response>   responder_;
    ProceedFunc                                 proceed_;
    CallStatus                                  status_;
};
// 高性能异步服务器（零手写状态机）
class AsyncServer final {
public:
    AsyncServer();
    ~AsyncServer();
    void Run(uint16_t port = 0);          // 0 = 随机端口
    void Shutdown();                      // 优雅停机
    uint16_t ListenPort() const { return listen_port_; }

    // 业务注册点：只写函数，不写类
    using SubmitTaskFunc = std::function<void(Task*, TaskResponse*)>;
    void SetSubmitTaskHandler(SubmitTaskFunc f) { submit_task_ = std::move(f); }

private:
    void DriveCompletionQueue(grpc::ServerCompletionQueue* cq);
    void OnSubmitTask(AsyncCallContext<AsyncTaskService, Task, TaskResponse>* ctx);

    std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> cqs_;
    AsyncTaskService service_;
    std::unique_ptr<grpc::Server> server_;
    std::atomic<bool> shutdown_{false};
    std::vector<std::thread> cq_threads_;
    int listen_port_ = 0;

    SubmitTaskFunc submit_task_;          // 业务回调
};

} // namespace dts