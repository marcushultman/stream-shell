#pragma once

#include "stream-shell/operand.h"

inline auto lookupField(Value input, ranges::forward_range auto path) -> Stream {
  if (ranges::empty(path)) {
    return ranges::yield(input);
  }
  auto value = ranges::fold_left(
      path,
      std::get_if<google::protobuf::Value>(&input),
      [&](google::protobuf::Value *json, auto field) -> google::protobuf::Value * {
        if (json && json->has_struct_value()) {
          auto *fields = json->mutable_struct_value()->mutable_fields();
          if (auto it = fields->find(field | ranges::to<std::string>); it != fields->end()) {
            return &it->second;
          }
        }
        return nullptr;
      });
  if (!value) {
    // Don't treat this as error - just omit this value
    return Stream();
  } else if (value->has_list_value()) {
    auto list = std::move(*value->mutable_list_value());
    return ranges::views::iota(0, list.values().size()) |
           ranges::views::transform([list = std::move(list)](auto i) { return list.values(i); });
  }
  return ranges::yield(std::move(*value));
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
  return lookupField(std::move(value), path);
}
