#pragma once

#include "stream-shell/operand_op.h"
#include "stream-shell/to_stream.h"

inline Stream prepend(ToStream &&to_stream,
                      Result<Value> &&value,
                      ranges::bidirectional_range auto args) {
  if (!value) {
    return ranges::views::single(std::unexpected(value.error()));
  }
  if (ranges::empty(args)) {
    return ranges::views::single(std::unexpected(Error::kMissingOperand));
  }
  // todo: fold_left
  return std::visit(to_stream,
                    std::visit(ValueTransform(ValueOp<std::plus<>>()), ranges::back(args), *value));
}

inline Stream prepend(ToStream &&to_stream, Stream input, ranges::bidirectional_range auto args) {
  return ranges::views::concat(
      args | ranges::views::for_each([=](auto &arg) { return std::visit(to_stream, arg); }), input);
}
