#include "grpc_client.hpp"

namespace dts {


// ---------- 构造/析构 ----------
GrpcClient::GrpcClient(const std::string& target)
    : stub_(TaskService::NewStub(
          grpc::CreateChannel(target, grpc::InsecureChannelCredentials()))),
      thread_pool_(std::make_shared<ThreadPool>(4)) {
    cq_thread_ = std::thread([this] { CompleteRpc(); });
}

GrpcClient::~GrpcClient() {
    shutdown_ = true;
    cq_.Shutdown();
    if (cq_thread_.joinable()) cq_thread_.join();
}

// ---------- 异步完成分发 ----------
void GrpcClient::CompleteRpc() {
    void* tag = nullptr;
    bool ok = false;

    while (!shutdown_.load(std::memory_order_acquire)) {
        auto status = cq_.AsyncNext(&tag, &ok,
                                    gpr_time_add(gpr_now(GPR_CLOCK_REALTIME),
                                                 gpr_time_from_seconds(1, GPR_TIMESPAN)));

        switch (status) {
        case grpc::CompletionQueue::NextStatus::GOT_EVENT:
            static_cast<IAsyncTag*>(tag)->Proceed(ok);
            break;
        case grpc::CompletionQueue::NextStatus::SHUTDOWN:
            return;
        case grpc::CompletionQueue::NextStatus::TIMEOUT:
            break;
        }
    }
}

// ---------- 各 RPC 实现 ----------
std::future<Task> GrpcClient::submit_task_async(const Task& task, Callback cb) {
    if (!stub_) {
        auto prom = std::make_shared<std::promise<Task>>();
        prom->set_exception(std::make_exception_ptr(
            GrpcError(grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                   "stub_ is null (channel create failed or moved)"))));
        if (cb) cb(Task{}, grpc::Status(grpc::StatusCode::UNAVAILABLE, "stub_ null"));
        return prom->get_future();
    }

    auto promise = std::make_shared<std::promise<Task>>();
    auto *tag = new AsyncSubmitTag(promise, std::move(cb));

    auto future = promise->get_future();
    tag->reader = stub_->PrepareAsyncSubmitTask(&tag->context,
                                                TaskToProto(task), &cq_);
    tag->reader->StartCall();


    return future;
}

Task GrpcClient::submit_task_sync(const Task& task) {
    if (!stub_) {
        throw GrpcError(grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                     "channel not created / connection refused"));
    }

    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() +
                     std::chrono::seconds(5));  // 5 秒超时

    TaskResponse resp;
    grpc::Status st = stub_->SubmitTask(&ctx, TaskToProto(task), &resp);

    if (!st.ok()) {
        std::cerr << "[ERROR] SubmitTask failed: "
                  << st.error_code() << " - " << st.error_message() << std::endl;
        throw GrpcError(st);
    }

    return TaskFromProto(resp.task());
}

std::future<bool> GrpcClient::cancel_task_async(const std::string& task_id) {
    auto tag = std::make_unique<AsyncCancelTag>();
    tag->request.set_task_id(task_id);
    tag->reader = stub_->PrepareAsyncCancelTask(&tag->context,
                                                tag->request, &cq_);
    tag->reader->StartCall();
    tag->reader->Finish(&tag->response, &tag->status, tag.release());
    return tag->promise.get_future();
}

std::future<Task> GrpcClient::query_status_async(const std::string& task_id) {
    auto tag = std::make_unique<AsyncQueryTag>();
    tag->request.set_task_id(task_id);
    tag->reader = stub_->PrepareAsyncQueryStatus(&tag->context,
                                                 tag->request, &cq_);
    tag->reader->StartCall();
    tag->reader->Finish(&tag->response, &tag->status, tag.release());
    return tag->promise.get_future();
}

bool GrpcClient::cancel_task(const std::string& task_id) {
    return cancel_task_async(task_id).get();
}
Task GrpcClient::query_status(const std::string& task_id) {
    return query_status_async(task_id).get();
}

// 流式监听
void GrpcClient::listen_results(const std::string& client_id, Callback cb) {
    auto tag = std::make_unique<AsyncListenTag>(std::move(cb));
    tag->request.set_client_id(client_id);
    tag->reader = stub_->PrepareAsyncListenResults(&tag->context,
                                                   tag->request, &cq_);
    tag->reader->StartCall(tag.release()); 
}

}   // namespace dts