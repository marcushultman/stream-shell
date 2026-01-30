#pragma once

#include "stream_parser.h"
#include "variant_ext.h"

using Operand = variant_ext_t<Value, Stream, StreamRef>;

template <typename T>
concept IsValue = InVariant<Value, T>;

constexpr const std::string *getIfString(const Operand &op) {
  auto *val = std::get_if<google::protobuf::Value>(&op);
  return val && val->has_string_value() ? &val->string_value() : nullptr;
}
