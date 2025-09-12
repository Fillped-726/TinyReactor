distributed-task-system/
├── README.md                    # 项目概述、架构图、快速开始指南、性能指标
├── CMakeLists.txt              # 主CMake配置文件
├── .gitignore                  # Git忽略文件
├── .clang-format              # 代码格式化配置
├── .clang-tidy                # 静态分析配置
│
├── docs/                       # 详细的技术文档
│   ├── api/                   # API文档
│   │   ├── rest-api.md
│   │   ├── grpc-api.md
│   │   └── websocket-api.md
│   ├── design/                # 设计文档
│   │   ├── architecture.md
│   │   ├── system-design.md
│   │   └── database-design.md
│   ├── deployment/            # 部署文档
│   │   ├── docker-deployment.md
│   │   ├── kubernetes-deployment.md
│   │   └── performance-tuning.md
│   └── user-guide.md          # 使用指南
│
├── src/                        # 项目源代码
│   ├── api-server/            # API服务器
│   │   ├── include/
│   │   │   ├── api_server.hpp
│   │   │   ├── request_handler.hpp
│   │   │   └── response_builder.hpp
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── api_server.cpp
│   │   │   ├── request_handler.cpp
│   │   │   └── response_builder.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── scheduler/             # 调度服务器
│   │   ├── node-manager/
│   │   │   ├── include/
│   │   │   │   ├── node_manager.hpp
│   │   │   │   ├── node_registry.hpp
│   │   │   │   └── health_checker.hpp
│   │   │   ├── src/
│   │   │   │   ├── node_manager.cpp
│   │   │   │   ├── node_registry.cpp
│   │   │   │   └── health_checker.cpp
│   │   │   └── CMakeLists.txt
│   │   │
│   │   ├── task-scheduler/
│   │   │   ├── include/
│   │   │   │   ├── task_scheduler.hpp
│   │   │   │   ├── scheduling_algorithm.hpp
│   │   │   │   └── task_queue.hpp
│   │   │   ├── src/
│   │   │   │   ├── task_scheduler.cpp
│   │   │   │   ├── scheduling_algorithm.cpp
│   │   │   │   └── task_queue.cpp
│   │   │   └── CMakeLists.txt
│   │   │
│   │   └── resource-monitor/
│   │       ├── include/
│   │       │   ├── resource_monitor.hpp
│   │       │   ├── metrics_collector.hpp
│   │       │   └── resource_analyzer.hpp
│   │       ├── src/
│   │       │   ├── resource_monitor.cpp
│   │       │   ├── metrics_collector.cpp
│   │       │   └── resource_analyzer.cpp
│   │       └── CMakeLists.txt
│   │
│   ├── worker/                # 工作节点
│   │   ├── task-executor/
│   │   │   ├── include/
│   │   │   │   ├── task_executor.hpp
│   │   │   │   ├── task_runner.hpp
│   │   │   │   └── result_handler.hpp
│   │   │   ├── src/
│   │   │   │   ├── task_executor.cpp
│   │   │   │   ├── task_runner.cpp
│   │   │   │   └── result_handler.cpp
│   │   │   └── CMakeLists.txt
│   │   │
│   │   └── resource-reporter/
│   │       ├── include/
│   │       │   ├── resource_reporter.hpp
│   │       │   ├── system_info.hpp
│   │       │   └── metrics_sender.hpp
│   │       ├── src/
│   │       │   ├── resource_reporter.cpp
│   │       │   ├── system_info.cpp
│   │       │   └── metrics_sender.cpp
│   │       └── CMakeLists.txt
│   │
│   ├── storage/               # 存储层
│   │   ├── redis/
│   │   │   ├── include/
│   │   │   │   ├── redis_client.hpp
│   │   │   │   ├── redis_connection_pool.hpp
│   │   │   │   └── redis_cache.hpp
│   │   │   ├── src/
│   │   │   │   ├── redis_client.cpp
│   │   │   │   ├── redis_connection_pool.cpp
│   │   │   │   └── redis_cache.cpp
│   │   │   └── CMakeLists.txt
│   │   │
│   │   └── leveldb/
│   │       ├── include/
│   │       │   ├── leveldb_store.hpp
│   │       │   ├── log_writer.hpp
│   │       │   └── log_reader.hpp
│   │       ├── src/
│   │       │   ├── leveldb_store.cpp
│   │       │   ├── log_writer.cpp
│   │       │   └── log_reader.cpp
│   │       └── CMakeLists.txt
│   │
│   └── common/                # 公共组件
│       ├── include/
│       │   ├── logger.hpp
│       │   ├── config.hpp
│       │   ├── utils.hpp
│       │   └── exceptions.hpp
│       ├── src/
│       │   ├── logger.cpp
│       │   ├── config.cpp
│       │   ├── utils.cpp
│       │   └── exceptions.cpp
│       └── CMakeLists.txt
│
├── tests/                      # 测试代码
│   ├── unit/
│   │   ├── api-server-test/
│   │   ├── scheduler-test/
│   │   ├── worker-test/
│   │   └── storage-test/
│   ├── integration/
│   │   ├── system-integration-test/
│   │   ├── performance-test/
│   │   └── stress-test/
│   ├── CMakeLists.txt
│   └── test_main.cpp
│
├── examples/                   # 示例代码和配置文件
│   ├── config/
│   │   ├── api-server-config.json
│   │   ├── scheduler-config.json
│   │   ├── worker-config.json
│   │   └── storage-config.json
│   └── sample-code/
│       ├── client-example.cpp
│       └── admin-tool.cpp
│
├── configs/                    # 配置文件模板
│   ├── development/
│   ├── staging/
│   └── production/
│
├── cmake/                      # CMake模块
│   ├── CompilerWarnings.cmake
│   ├── StandardProjectSettings.cmake
│   └── FindDependencies.cmake
│
├── scripts/                    # 脚本文件
│   ├── build.sh               # 构建脚本
│   ├── test.sh                # 测试脚本
│   ├── deploy.sh              # 部署脚本
│   └── setup-dev-env.sh       # 开发环境设置脚本
│
├── third_party/               # 第三方依赖
│   ├── quill/                 # 日志库
│   ├── cpp-httplib/           # HTTP服务器
│   ├── redis-cpp/             # Redis客户端
│   └── leveldb/               # LevelDB
│
└── build/                     # 构建输出目录（git忽略）