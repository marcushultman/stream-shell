#include "config.h"

#include <variant>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/wrappers.pb.h>
#include "lift.h"
#include "operand.h"
#include "stream_parser.h"

namespace {

struct Buffer {
  google::protobuf::Struct json;
  std::deque<google::protobuf::Any> record = {google::protobuf::Any()};

  google::protobuf::Struct *merge_target = &json;
  google::protobuf::ListValue *positionals = (*json.mutable_fields())["@"].mutable_list_value();
};

bool merge(Env &, Buffer &buffer, const google::protobuf::BytesValue &bytes) {
  auto *arg = buffer.positionals->add_values()->mutable_string_value();
  return google::protobuf::json::MessageToJsonString(bytes, arg).ok();
}

bool merge(Env &, Buffer &buffer, const google::protobuf::Value &val) {
  if (val.has_struct_value()) {
    buffer.merge_target->MergeFrom(val.struct_value());

  } else {
    *buffer.positionals->add_values() = val;
    buffer.record.emplace_back();
    std::string subcommand;
    if (!google::protobuf::json::MessageToJsonString(val, &subcommand).ok()) {
      return false;
    }
    buffer.merge_target =
        (*buffer.merge_target->mutable_fields())[subcommand].mutable_struct_value();
  }
  return true;
}

bool merge(Env &, Buffer &buffer, const google::protobuf::Any &any) {
  if (buffer.record.back().type_url().empty()) {
    buffer.record.back().set_type_url(any.type_url());
  }
  if (buffer.record.back().type_url() != any.type_url()) {
    return false;
  }
  buffer.record.back().mutable_value()->append_range(any.value());
  return true;
}

bool merge(Env &env, Buffer &buffer, const Value &value) {
  return std::visit([&](auto &value) { return merge(env, buffer, value); }, value);
}

bool merge(Env &env, Buffer &buffer, const Stream &stream) {
  return lift(stream)
      .transform([&](auto values) {
        return std::ranges::all_of(values, [&](auto val) { return merge(env, buffer, val); });
      })
      .value_or(false);
}

bool merge(Env &env, Buffer &buffer, const StreamRef &ref) {
  auto f = env.getEnv(ref);
  return f && merge(env, buffer, f({}));
}

bool merge(Env &, Buffer &buffer, const Word &word) {
  buffer.positionals->add_values()->set_string_value(Token(word.value) | ranges::to<std::string>);
  return true;
}

auto to_string(auto value) {
  std::string arg;
  assert(google::protobuf::util::MessageToJsonString(value, &arg).ok());
  return arg;
}

}  // namespace

Result<google::protobuf::Struct> toConfig(Env &env, std::span<const Operand> operands) {
  Buffer buffer;

  for (const Operand &op : operands) {
    if (auto ok = std::visit([&](auto &op) { return merge(env, buffer, op); }, op); !ok) {
      return std::unexpected(Error::kConfigError);
    }
  }

  for (buffer.merge_target = &buffer.json; const auto &[cmd, record] : ranges::views::zip(
                                               buffer.positionals->values(), buffer.record)) {
    std::string json;
    if (!google::protobuf::json::MessageToJsonString(cmd, &json).ok()) {
      return std::unexpected(Error::kConfigError);
    }

    buffer.merge_target = (*buffer.merge_target->mutable_fields())[json].mutable_struct_value();

    if (!record.type_url().empty()) {
      if (!google::protobuf::json::MessageToJsonString(record, &json).ok()) {
        return std::unexpected(Error::kConfigError);
      }
      if (!google::protobuf::json::JsonStringToMessage(json, &buffer.json).ok()) {
        return std::unexpected(Error::kConfigError);
      }
    }
  }

  return buffer.json;
}

std::vector<std::string> toArgs(const google::protobuf::Struct &config) {
  using namespace std::string_view_literals;

  std::vector<std::string> args;

  for (auto &[key, value] : config.fields()) {
    if (key == "@"sv && value.has_list_value()) {
      for (auto &value : value.list_value().values()) {
        args.push_back(to_string(value));
      }
    } else if (key.size() == 1 && value.bool_value()) {
      args.push_back("-" + key);
    } else if (value.bool_value()) {
      args.push_back("--" + key);
    } else {
      args.push_back("--" + key + "=" + to_string(value));
    }
  }
  return args;
}
