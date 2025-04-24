#pragma once

#include "stream-shell/operand_op.h"
#include "stream-shell/to_stream.h"

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

inline Stream get(ToStream &&to_stream, Value &&value, auto args) {
  Operand op = ranges::front(args);
  auto *word = std::get_if<Word>(&op);
  if (!word) {
    return ranges::views::single(std::unexpected(Error::kMissingOperand));
  }
  return std::visit(to_stream,
                    lookupField(std::move(value), word->value | ranges::views::split('.')));
}

inline Stream get(auto &&...) {
  return ranges::views::single(std::unexpected(Error::kInvalidOp));
}
