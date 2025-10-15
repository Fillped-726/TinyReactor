#pragma once
#include <glog/logging.h>

namespace dts {

inline void InitGlog(const char* argv0, bool unit_test = false) {
    google::InitGoogleLogging(argv0);
    if (unit_test) {
        // 测试模式：只打 ERROR 到 stderr，不产生文件
        FLAGS_logtostderr = 1;
        FLAGS_minloglevel = 1;   // 1=WARNING, 0=INFO
    } else {
        FLAGS_log_dir = "/var/log/dts";   // 按需修改
        FLAGS_max_log_size = 100;         // 100 MB 轮转
        FLAGS_stop_logging_if_full_disk = true;
        FLAGS_colorlogtostderr = true;
        google::InstallFailureSignalHandler(); // SIGSEGV 打印栈
    }
}

} // namespace dts