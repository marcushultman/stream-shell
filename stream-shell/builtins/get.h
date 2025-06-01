#pragma once

#include "stream-shell/operand.h"

inline auto lookupField(Value value, ranges::forward_range auto path) -> Value {
  if (ranges::empty(path)) {
    return value;
  }
  google::protobuf::Value empty;
  empty.mutable_list_value();

  if (!std::holds_alternative<google::protobuf::Value>(value)) {
    return empty;
  }
  return ranges::fold_left(
      path, std::get<google::protobuf::Value>(std::move(value)), [&](auto &&json, auto field) {
        if (json.has_struct_value()) {
          auto fields = std::move(*json.mutable_struct_value()->mutable_fields());
          if (auto it = fields.find(field | ranges::to<std::string>); it != fields.end()) {
            return std::move(it->second);
          }
        }
        return empty;
      });
}

inline Operand get(Value value, auto args) {
  if (ranges::size(args) != 1) {
    return ranges::yield(std::unexpected(Error::kMissingOperand));
  }
  Operand op = ranges::front(args);
  auto *word = std::get_if<Word>(&op);
  if (!word) {
    return ranges::yield(std::unexpected(Error::kMissingOperand));
  }
  auto path = word->value | ranges::views::split('.');
  return ranges::yield(lookupField(std::move(value), path));
}
