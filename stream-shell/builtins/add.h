#pragma once

#include "stream-shell/operand_op.h"

inline Stream add(Value value, auto args) {
  if (ranges::size(args) != 1) {
    return ranges::yield(std::unexpected(Error::kMissingOperand));
  }
  return std::visit(ValueTransform(ValueOp<std::plus<>>()), std::move(value), ranges::front(args));
}
