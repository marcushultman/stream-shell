#pragma once

#include "stream-shell/operand_op.h"
#include "stream-shell/to_stream.h"

inline Stream add(ToStream &&to_stream, const Value &value, auto args) {
  if (ranges::size(args) != 1) {
    return ranges::yield(std::unexpected(Error::kMissingOperand));
  }
  return std::visit(to_stream,
                    std::visit(ValueTransform(ValueOp<std::plus<>>()), value, ranges::front(args)));
}

inline Stream add(ToStream &&to_stream, Stream input, auto args) {
  return ranges::views::concat(input, args | ranges::views::for_each(to_stream));
}
