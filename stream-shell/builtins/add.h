#pragma once

#include "stream-shell/operand_op.h"

inline Stream add(Value value, const google::protobuf::Struct &config) {
  if (auto it = config.fields().find("@"); it != config.fields().end()) {
    for (auto &arg : it->second.list_value().values()) {
      return std::visit(ValueTransform(ValueOp<std::plus<>>()), std::move(value), Value(arg));
    }
  }
  return ranges::yield(value);
}
