#pragma once
#include "task.pb.h"
#include "task.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/support/async_stream.h>
#include <future>
#include <memory>
#include <atomic>
#include <functional>
#include "thread_pool.hpp"
#include "task.hpp"
#include "utils.hpp"
#include <mutex>

namespace dts {

// 前置声明
using PbTask = ::dts::proto::Task;
using TaskResponse = ::dts::proto::TaskResponse;
using CancelRequest = ::dts::proto::CancelRequest;
using CancelResponse = ::dts::proto::CancelResponse;
using QueryRequest = ::dts::proto::QueryRequest;
using SubscribeRequest = ::dts::proto::SubscribeRequest;
using TaskResult = ::dts::proto::TaskResult;
using TaskService = ::dts::proto::TaskService;

class GrpcClient;
using Callback = std::function<void(const Task& result, grpc::Status status)>;

// ---------- 异常类 ----------
class GrpcError : public std::runtime_error {
public:
    explicit GrpcError(const grpc::Status& s)
        : std::runtime_error(s.error_message()), code_(s.error_code()) {}
    grpc::StatusCode code() const noexcept { return code_; }
private:
    grpc::StatusCode code_;
};

// ---------- 异步上下文 ----------

struct IAsyncTag { 
    virtual ~IAsyncTag() = default;
    grpc::Status status;
    virtual void Proceed(bool ok) = 0;
    virtual void ProceedImpl(bool ok) = 0;
 };

template<typename Tag>
struct AsyncTagBase : IAsyncTag {
    void Proceed(bool ok) override {           
        static_cast<Tag*>(this)->ProceedImpl(ok);
    }
protected:
    ~AsyncTagBase() = default; 
};

struct AsyncSubmitTag : AsyncTagBase<AsyncSubmitTag> {
    explicit AsyncSubmitTag(std::shared_ptr<std::promise<Task>> p,
                            Callback cb = {})
        : promise(std::move(p)), callback(std::move(cb)) {}
    void ProceedImpl(bool ok) {
        switch (step_) {
        case kLaunch:
            if (!ok) {
                status = grpc::Status(grpc::StatusCode::INTERNAL, "StartCall failed");
                step_ = kFinish;          // 直接跳到结束
            } else {
                step_ = kFinish;
            }
            // 无论 ok 与否都调 Finish，让 gRPC 再回包一次
            reader->Finish(&response, &status, this);
            break;

        case kFinish:
            if (!ok) {
                // Finish 本身失败，覆盖 status
                status = grpc::Status(grpc::StatusCode::INTERNAL, "Finish cq !ok");
            }
            SetResult();
            delete this;
            break;
        }
    }
    enum Step { kLaunch, kFinish } step_{kLaunch};
    TaskResponse response;
    std::unique_ptr<grpc::ClientAsyncResponseReader<TaskResponse>> reader;
    grpc::ClientContext context;
    std::shared_ptr<std::promise<Task>> promise;
    Callback callback;

    void SetResult() {
        std::call_once(once_, [&] {
            if (status.ok()) {
                Task task = TaskFromProto(response.task());
                promise->set_value(std::move(task));
                if (callback) callback(task, status);
            } else {
                promise->set_exception(std::make_exception_ptr(GrpcError(status)));
                if (callback) callback(Task{}, status);
            }
        });
    }

private:
    std::once_flag once_;
};

struct AsyncCancelTag  : AsyncTagBase<AsyncCancelTag> {
    AsyncCancelTag() = default;
    void ProceedImpl(bool ok) {
        if (step_ == kLaunch && ok) {
            step_ = kFinish;
            reader->Finish(&response, &status, this);
            return;
        }
        if (!ok && step_ == kLaunch) status = grpc::Status(grpc::StatusCode::INTERNAL, "cq !ok");
        promise.set_value(status.ok() && response.success());
        delete this;
    }

    enum Step { kLaunch, kFinish } step_{kLaunch};
    CancelRequest request;
    CancelResponse response;
    std::promise<bool> promise;
    std::unique_ptr<grpc::ClientAsyncResponseReader<CancelResponse>> reader;
    grpc::ClientContext context;
};

struct AsyncQueryTag   : AsyncTagBase<AsyncQueryTag> {
    AsyncQueryTag() = default;
    void ProceedImpl(bool ok) {
        if (step_ == kLaunch && ok) {
            step_ = kFinish;
            reader->Finish(&response, &status, this);
            return;
        }
        if (!ok && step_ == kLaunch) status = grpc::Status(grpc::StatusCode::INTERNAL, "cq !ok");
        SetResult();
        delete this;
    }

    enum Step { kLaunch, kFinish } step_{kLaunch};
    QueryRequest request;
    PbTask response;
    std::promise<Task> promise;
    std::unique_ptr<grpc::ClientAsyncResponseReader<PbTask>> reader;
    grpc::ClientContext context;

    void SetResult() {
        std::call_once(once_, [&] {
            if (status.ok()) {
                Task task = TaskFromProto(response);
                promise.set_value(std::move(task));
            } else {
                promise.set_exception(std::make_exception_ptr(GrpcError(status)));
            }
        });
    }

private:
    std::once_flag once_;
};

struct AsyncListenTag : AsyncTagBase<AsyncListenTag> {
    enum Step { kStart = 0, kRead, kFinish, kDone };

    explicit AsyncListenTag(Callback cb)
        : callback(std::move(cb)), step_(kStart) {}

    void ProceedImpl(bool ok) override {
        switch (step_) {
        /* ----------------------------------
         * 0. 启动：StartCall() 完成后第一次 Proceed
         * ---------------------------------- */
        case kStart:
            if (!ok) {                    // 连建立就失败
                TransitionToFinish(grpc::StatusCode::CANCELLED);
                return;
            }
            step_ = kRead;                // 正常建立，开始读
            reader->Read(&response, this);
            return;

        /* ----------------------------------
         * 1. 读消息：ok==true  读到一条
         *            ok==false 对端半关 / 网络出错
         * ---------------------------------- */
        case kRead:
            if (!ok) {                    // 早退
                TransitionToFinish(grpc::StatusCode::CANCELLED);
                return;
            }
            if (response.has_task()) {    // 正常业务数据
                Task task = TaskFromProto(response.task());
                if (callback) callback(task, grpc::Status::OK);
                response.Clear();
                reader->Read(&response, this);   // 继续读下一条
                return;
            }
            // response 没有 task → 服务端发 EOF
            TransitionToFinish(grpc::StatusCode::OK);
            return;

        /* ----------------------------------
         * 2. Finish 回调：拿到最终状态
         * ---------------------------------- */
        case kFinish:
            step_ = kDone;
            if (callback && !status.ok())   // 把最终错误带给用户
                callback(Task{}, status);
            delete this;                    // 唯一 suicide 点
            return;

        /* ----------------------------------
         * 3. 终态：永远不会走到
         * ---------------------------------- */
        case kDone:
            GPR_ASSERT(false);
        }
    }

    /* 统一入口：任何路径想结束流，都调到这儿 */
    void TransitionToFinish(grpc::StatusCode code) {
        step_ = kFinish;
        if (code != grpc::StatusCode::OK)
            status = grpc::Status(code, "stream aborted");
        reader->Finish(&status, this);   // 恰好一次 Finish
    }

    Callback callback;
    SubscribeRequest request;
    TaskResult response;
    std::unique_ptr<grpc::ClientAsyncReader<TaskResult>> reader;
    grpc::ClientContext context;
    Step step_;
};

// ---------- 客户端 ----------
class GrpcClient {
public:
    GrpcClient(const std::string& target);
    ~GrpcClient();

    void CompleteRpc();
    Task submit_task_sync(const Task& task);
    bool cancel_task(const std::string& task_id);
    Task query_status(const std::string& task_id);
    void listen_results(const std::string& client_id, Callback callback);
    std::future<Task> submit_task_async(const Task& task, Callback callback = nullptr);
    std::future<bool> cancel_task_async(const std::string& task_id);
    std::future<Task> query_status_async(const std::string& task_id);

private:
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<TaskService::Stub> stub_;
    grpc::CompletionQueue cq_;
    std::shared_ptr<ThreadPool> thread_pool_;
    std::atomic<bool> shutdown_{false};
    std::thread cq_thread_;
};

}   // namespace dts