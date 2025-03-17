#pragma once

#include <string_view>
#include <variant>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/wrappers.pb.h>

struct Value {
  using InnerValue = std::variant<google::protobuf::Any, google::protobuf::StringValue>;

  Value(std::string_view str) {
    google::protobuf::StringValue value;
    value.set_value(str);
    _value = value;
  }

  friend auto &operator<<(std::ostream &os, const Value &);

  InnerValue _value;
};

inline auto &operator<<(std::ostream &os, const Value &v) {
  return os << std::get<google::protobuf::StringValue>(v._value).value();
}
