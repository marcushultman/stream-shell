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
  struct Entry {
    std::string subcommand;
    google::protobuf::Any record;
  };
  std::vector<Entry> entries = {Entry{}};

  google::protobuf::Struct *merge_target = &json;
  bool positional_only = false;

  void append(const google::protobuf::Value &value) {
    *(*merge_target->mutable_fields())["@"].mutable_list_value()->add_values() = value;
  }
};

bool merge(Env &, Buffer &buffer, const google::protobuf::BytesValue &bytes) {
  google::protobuf::Value value;
  if (!google::protobuf::json::MessageToJsonString(bytes, value.mutable_string_value()).ok()) {
    return false;
  }
  buffer.append(value);
  return true;
}

bool merge(Env &, Buffer &buffer, const google::protobuf::Value &val) {
  if (buffer.positional_only) {
    buffer.append(val);

  } else if (val.has_string_value()) {
    if (val.string_value() == "--") {
      buffer.positional_only = true;
      buffer.append(val);
    } else {
      buffer.entries.push_back({val.string_value(), {}});
      buffer.merge_target =
          (*buffer.merge_target->mutable_fields())[val.string_value()].mutable_struct_value();
    }

  } else if (val.has_struct_value()) {
    buffer.merge_target->MergeFrom(val.struct_value());

  } else {
    buffer.append(val);
  }
  return true;
}

bool merge(Env &, Buffer &buffer, const google::protobuf::Any &any) {
  if (buffer.positional_only) {
    std::string json;
    google::protobuf::Value value;
    if (!google::protobuf::json::MessageToJsonString(any, &json).ok() ||
        !google::protobuf::json::JsonStringToMessage(json, &value).ok()) {
      return false;
    }
    buffer.append(value);
    return true;
  }
  if (buffer.entries.back().record.type_url().empty()) {
    buffer.entries.back().record.set_type_url(any.type_url());
  }
  if (buffer.entries.back().record.type_url() != any.type_url()) {
    return false;
  }
  buffer.entries.back().record.mutable_value()->append_range(any.value());
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
  return f && merge(env, buffer, f());
}

auto to_string(const auto &value) {
  std::string arg;
  const auto status = google::protobuf::util::MessageToJsonString(value, &arg);
  assert(status.ok());
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

  auto merge_record = [&](const google::protobuf::Any &record) {
    if (!record.type_url().empty()) {
      std::string json;
      google::protobuf::Struct fields;
      if (!google::protobuf::json::MessageToJsonString(record, &json).ok() ||
          !google::protobuf::json::JsonStringToMessage(json, &fields).ok()) {
        return false;
      }
      buffer.merge_target->MergeFrom(fields);
    }
    return true;
  };

  buffer.merge_target = &buffer.json;
  if (!merge_record(buffer.entries.front().record)) {
    return std::unexpected(Error::kConfigError);
  }
  for (const auto &[subcommand, record] : buffer.entries | ranges::views::drop(1)) {
    buffer.merge_target =
        (*buffer.merge_target->mutable_fields())[subcommand].mutable_struct_value();
    if (!merge_record(record)) {
      return std::unexpected(Error::kConfigError);
    }
  }

  return buffer.json;
}

std::vector<std::string> toArgs(const google::protobuf::Struct &config) {
  using namespace std::string_view_literals;

  std::vector<std::string> args;

  // Options precede positionals so that a retained "--" cannot turn them
  // into positional arguments. Descend into subcommands only after this level.
  for (const auto &[key, value] : config.fields()) {
    if ((key == "@"sv && value.has_list_value()) || value.has_struct_value()) {
      continue;
    }
    if (value.has_bool_value() && value.bool_value()) {
      args.push_back((key.size() == 1 ? "-" : "--") + key);
    } else {
      args.push_back("--" + key + "=" +
                     (value.has_string_value() ? value.string_value() : to_string(value)));
    }
  }

  if (auto it = config.fields().find("@");
      it != config.fields().end() && it->second.has_list_value()) {
    for (const auto &value : it->second.list_value().values()) {
      args.push_back(value.has_string_value() ? value.string_value() : to_string(value));
    }
  }

  for (const auto &[key, value] : config.fields()) {
    if (value.has_struct_value()) {
      args.push_back(key);
      auto nested = toArgs(value.struct_value());
      args.insert(args.end(), nested.begin(), nested.end());
    }
  }
  return args;
}
