#include "utils.hpp"

namespace dts {

void JsonToStruct(const json& j, google::protobuf::Struct* proto) {
    proto->Clear();
    for (auto& [k, v] : j.items()) {
        google::protobuf::Value pv;
        if (v.is_boolean())       pv.set_bool_value(v);
        else if (v.is_number())   pv.set_number_value(v);
        else if (v.is_string())   pv.set_string_value(v);
        else if (v.is_null()) pv.set_null_value(google::protobuf::NULL_VALUE);
        else if (v.is_array() || v.is_object()) {
            JsonToStruct(v, pv.mutable_struct_value());
        }
        (*proto->mutable_fields())[k] = std::move(pv);
    }
}

json StructToJson(const google::protobuf::Struct& proto) {
    json j;
    for (auto& [k, v] : proto.fields()) {
        switch (v.kind_case()) {
            case google::protobuf::Value::kBoolValue:   j[k] = v.bool_value(); break;
            case google::protobuf::Value::kNumberValue: j[k] = v.number_value(); break;
            case google::protobuf::Value::kStringValue: j[k] = v.string_value(); break;
            case google::protobuf::Value::kStructValue: j[k] = StructToJson(v.struct_value()); break;
            default: break;
        }
    }
    return j;
}

PbTask TaskToProto(const Task& task) {
    PbTask proto;
    proto.set_task_id(task.task_id);
    proto.set_client_id(task.client_id);
    proto.set_priority(task.priority);
    proto.set_state(static_cast<PbTaskState>(task.state));
    proto.set_func_name(task.func_name);

    JsonToStruct(task.func_params, proto.mutable_func_params());
    proto.mutable_required()->set_cpu_core(task.required.cpu_core);
    proto.mutable_required()->set_mem_mb(task.required.mem_mb);
    proto.mutable_shard()->set_shard_id(task.shard.shard_id);
    proto.mutable_shard()->set_total_shards(task.shard.total_shards);

    proto.set_timeout_ms(task.timeout_ms);
    proto.set_max_retry(task.max_retry);
    proto.set_retry_count(task.retry_count);
    proto.set_submit_ts(task.submit_ts);
    proto.set_start_ts(task.start_ts);
    proto.set_finish_ts(task.finish_ts);

    JsonToStruct(task.result, proto.mutable_result());
    proto.set_error_msg(task.error_msg);
    return proto;
}

Task TaskFromProto(const PbTask& proto) {
    Task task;
    task.task_id     = proto.task_id();
    task.client_id   = proto.client_id();
    task.priority    = proto.priority();
    task.state       = static_cast<TaskState>(proto.state());
    task.func_name   = proto.func_name();

    task.func_params = StructToJson(proto.func_params());
    task.required.cpu_core = proto.required().cpu_core();
    task.required.mem_mb   = proto.required().mem_mb();
    task.shard.shard_id     = proto.shard().shard_id();
    task.shard.total_shards = proto.shard().total_shards();

    task.timeout_ms  = proto.timeout_ms();
    task.max_retry   = proto.max_retry();
    task.retry_count = proto.retry_count();
    task.submit_ts   = proto.submit_ts();
    task.start_ts    = proto.start_ts();
    task.finish_ts   = proto.finish_ts();

    task.result      = StructToJson(proto.result());
    task.error_msg   = proto.error_msg();
    return task;
}

} // namespace dts