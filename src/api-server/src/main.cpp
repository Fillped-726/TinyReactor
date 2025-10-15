#include "api_server.hpp"
#include "logger.hpp"          // 上一段给的 glog 封装
#include <csignal>
#include <iostream>

std::unique_ptr<dts::AsyncServer> g_server;

// 信号处理器：只改原子变量，逻辑留在主线程
static std::atomic<bool> g_shutdown{false};
static void signal_handler(int sig) {
    LOG(WARNING) << "Caught signal " << sig << ", shutting down...";
    g_shutdown = true;
    if (g_server) g_server->Shutdown();
}

int main(int argc, char* argv[]) {
    // 1. 日志初始化
    dts::InitGlog(argv[0], false);   // false = 生产模式

    // 2. 信号注册
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 3. 读端口 / 线程数
    uint16_t port = 0;                       // 默认随机
    if (const char* p = std::getenv("DTS_PORT")) port = static_cast<uint16_t>(std::stoi(p));

    // 4. 启动服务器
    g_server = std::make_unique<dts::AsyncServer>();
    g_server->Run(port);
    LOG(INFO) << "AsyncServer running on port " << g_server->ListenPort();

    // 5. 阻塞到收到信号
    while (!g_shutdown) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 6. 优雅退出（RAII 里已 join，这里再保险一次）
    g_server->Shutdown();
    LOG(INFO) << "AsyncServer exited cleanly";
    return 0;
}