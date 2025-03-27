#pragma once

#include <optional>
#include <string_view>
#include <range/v3/all.hpp>
#include "operand.h"
#include "stream_parser.h"

inline std::optional<Stream> findBuiltin(const Word &cmd) {
  if (cmd.value.starts_with("iota")) {
    return ranges::views::iota(1) | ranges::views::transform([](auto i) {
             google::protobuf::Value val;
             val.set_string_value("🐑 " + std::to_string(i));
             return val;
           });
  }
  if (cmd.value == "exit") {
    return ranges::views::generate([] { return exit(0), Value(); });
  }
  return {};
}
