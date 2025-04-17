#pragma once

#include <optional>
#include <string_view>
#include <range/v3/all.hpp>
#include "builtins/add.h"
#include "builtins/now.h"
#include "closure.h"
#include "to_stream.h"

using namespace std::string_view_literals;

// todo: pass args here and return Operand (since builtins may fail)
inline std::optional<Stream> runBuiltin(ToStream &&to_stream,
                                        auto &&input,
                                        Word cmd,
                                        ranges::bidirectional_range auto args) {
  if (ranges::starts_with(cmd.value, "iota"sv)) {
    return ranges::views::iota(1) | ranges::views::transform([](auto i) {
             google::protobuf::Value val;
             val.set_string_value("🐑 " + std::to_string(i));
             return val;
           });
  } else if (ranges::starts_with(cmd.value, "add"sv)) {
    return add(to_stream, std::forward<decltype(input)>(input), args);
  } else if (ranges::starts_with(cmd.value, "now"sv)) {
    return now();
  }
  if (cmd.value == "exit") {
    return ranges::views::generate([] { return exit(0), Value(); });
  }
  return {};
}
