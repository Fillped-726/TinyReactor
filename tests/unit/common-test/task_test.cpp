#include <gtest/gtest.h>
#include "task.hpp"
#include <chrono>

namespace dts {

// 测试夹具：设置一个默认任务
class TaskSerializationTest : public ::testing::Test {
protected:
    Task task;

    void SetUp() override {
        task.task_id = "uuid-1234";
        task.client_id = "client-001";
        task.priority = 5;
        task.state = TaskState::PENDING;
        task.func_name = "fib";
        task.func_params = {{"n", 10}, {"extra", "test"}};  // 嵌套 JSON
        task.required = {2.5, 1024};
        task.shard = {0, 1};
        task.timeout_ms = 30000;
        task.max_retry = 3;
        task.retry_count = 0;
        task.submit_ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        task.start_ts = 0;
        task.finish_ts = 0;
        task.result = {{"output", 55}};  // 模拟结果
        task.error_msg = "";
    }
};

// 测试正常序列化和反序列化
TEST_F(TaskSerializationTest, SerializeAndDeserialize) {
    // 序列化
    nlohmann::json json_task = task;
    std::string serialized = json_task.dump();

    // 反序列化
    nlohmann::json parsed_json = nlohmann::json::parse(serialized);
    Task deserialized_task = parsed_json.get<Task>();

    // 验证字段
    EXPECT_EQ(deserialized_task.task_id, task.task_id);
    EXPECT_EQ(deserialized_task.client_id, task.client_id);
    EXPECT_EQ(deserialized_task.priority, task.priority);
    EXPECT_EQ(deserialized_task.state, task.state);
    EXPECT_EQ(deserialized_task.func_name, task.func_name);
    EXPECT_EQ(deserialized_task.func_params["n"], task.func_params["n"]);
    EXPECT_EQ(deserialized_task.func_params["extra"], task.func_params["extra"]);
    EXPECT_DOUBLE_EQ(deserialized_task.required.cpu_core, task.required.cpu_core);
    EXPECT_EQ(deserialized_task.required.mem_mb, task.required.mem_mb);
    EXPECT_EQ(deserialized_task.shard.shard_id, task.shard.shard_id);
    EXPECT_EQ(deserialized_task.shard.total_shards, task.shard.total_shards);
    EXPECT_EQ(deserialized_task.timeout_ms, task.timeout_ms);
    EXPECT_EQ(deserialized_task.max_retry, task.max_retry);
    EXPECT_EQ(deserialized_task.retry_count, task.retry_count);
    EXPECT_EQ(deserialized_task.submit_ts, task.submit_ts);
    EXPECT_EQ(deserialized_task.start_ts, task.start_ts);
    EXPECT_EQ(deserialized_task.finish_ts, task.finish_ts);
    EXPECT_EQ(deserialized_task.result["output"], task.result["output"]);
    EXPECT_EQ(deserialized_task.error_msg, task.error_msg);
}

// 测试缺失字段的错误处理
TEST_F(TaskSerializationTest, MissingFields) {
    nlohmann::json json_task = task;
    json_task.erase("task_id");  // 移除必需字段
    EXPECT_THROW(json_task.get<Task>(), nlohmann::json::exception);
}

// 测试复杂 func_params
TEST_F(TaskSerializationTest, ComplexFuncParams) {
    task.func_params = {
        {"n", 10},
        {"config", {{"key1", "value1"}, {"key2", 42}}}
    };
    nlohmann::json json_task = task;
    std::string serialized = json_task.dump();

    nlohmann::json parsed_json = nlohmann::json::parse(serialized);
    Task deserialized_task = parsed_json.get<Task>();

    EXPECT_EQ(deserialized_task.func_params["n"], 10);
    EXPECT_EQ(deserialized_task.func_params["config"]["key1"], "value1");
    EXPECT_EQ(deserialized_task.func_params["config"]["key2"], 42);
}

}  // namespace dts

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}