#include "api_server.hpp"
#include <iostream>

namespace dts {

/* ---------- AsyncCallContext 实现 ---------- */
template <typename S, typename Rq, typename Rp>
void AsyncCallContext<S, Rq, Rp>::RequestNext() {
    status_ = CallStatus::CREATE;
    service_->RequestSubmitTask(&ctx_, &request_, &responder_, cq_, cq_, this);
}

/* 显式实例化（防止模板多次定义） */
template class AsyncCallContext<AsyncTaskService, Task, TaskResponse>;

/* ---------- AsyncServer 实现 ---------- */
AsyncServer::AsyncServer() = default;
AsyncServer::~AsyncServer() { Shutdown(); }

void AsyncServer::Run(uint16_t port) {
    std::string addr = port ? "0.0.0.0:" + std::to_string(port) : "0.0.0.0:0";
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials(), &listen_port_);
    builder.RegisterService(&service_);

    size_t cq_count = std::thread::hardware_concurrency();
    for (size_t i = 0; i < cq_count; ++i) {
        cqs_.emplace_back(builder.AddCompletionQueue());
    }

    server_ = builder.BuildAndStart();
    LOG(INFO) << "AsyncServer listening on " << listen_port_;

    int c = std::thread::hardware_concurrency() * 2;
    const char* env = std::getenv("DTS_INITIAL_CONTEXT");
    if (env) c = std::stoi(env);

    for (int i = 0; i < c; ++i) {
        auto* cq = cqs_[i % cq_count].get();
        new AsyncCallContext<AsyncTaskService, Task, TaskResponse>(
            &service_, cq,
            [this](auto* ctx) { OnSubmitTask(ctx); });
    }

    cq_threads_.reserve(cq_count);
    for (auto& cq : cqs_) {
        cq_threads_.emplace_back([this, cq_ptr = cq.get()] {
            DriveCompletionQueue(cq_ptr);
        });
    }
}

void AsyncServer::Shutdown() {
    shutdown_ = true;
    server_->Shutdown();
    for (auto& cq : cqs_) cq->Shutdown();
    for (auto& t : cq_threads_)
        if (t.joinable()) t.join();
}

void AsyncServer::DriveCompletionQueue(grpc::ServerCompletionQueue* cq) {
    void* tag;
    bool ok;
    while (true) {
        // 被 Shutdown() 唤醒后返回 false
        if (!cq->Next(&tag, &ok)) break;
        if (tag) static_cast<AsyncCallContext<AsyncTaskService,Task,TaskResponse>*>(tag)->Proceed(ok);
    }
    // 排空剩余事件
    while (cq->Next(&tag, &ok)) {
        if (tag) static_cast<AsyncCallContext<AsyncTaskService,Task,TaskResponse>*>(tag)->Proceed(ok);
    }
}

/* ---------- 业务逻辑 = 普通函数 ---------- */
void AsyncServer::OnSubmitTask(AsyncCallContext<AsyncTaskService, Task, TaskResponse>* ctx) {
    // 用户注册的高性能回调（无状态机噪音）
    ctx->response_.mutable_task()->CopyFrom(ctx->request_);
    ctx->response_.mutable_task()->set_state(dts::proto::SUCCESS);
}

} // namespace dts