#pragma once

#include "stream_parser.h"

#include <string_view>
#include "variant_ext.h"

struct Word {
  Word(std::string_view value) : value{value} {}
  std::string_view value;
  std::strong_ordering operator<=>(const Word &) const = default;
};

using Operand = variant_ext_t<Value, Stream, StreamRef, Word>;

inline auto toOperand(Value &&value) -> Operand {
  return std::visit([](auto &&value) -> Operand { return value; }, std::move(value));
}
