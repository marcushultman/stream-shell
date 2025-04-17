#pragma once

#include "stream-shell/operand_op.h"
#include "stream-shell/to_stream.h"

inline Stream add(ToStream &to_stream,
                  Result<Value> &&value,
                  ranges::bidirectional_range auto args) {
  if (!value) {
    return ranges::views::single(std::unexpected(value.error()));
  }
  return std::visit(
      to_stream, std::visit(ValueTransform(ValueOp<std::plus<>>()), *value, ranges::front(args)));
}

inline Stream add(ToStream &to_stream, Stream input, ranges::bidirectional_range auto args) {
  return ranges::views::concat(
      input, args | ranges::views::for_each([=](auto &arg) { return std::visit(to_stream, arg); }));
}
