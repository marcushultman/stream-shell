#pragma once

#include <optional>
#include <string_view>
#include <range/v3/all.hpp>
#include "stream_parser.h"

inline std::optional<Stream> findBuiltin(std::string_view cmd) {
  if (cmd.starts_with("iota")) {
    return ranges::views::iota(1) | ranges::views::transform([](auto i) {
             google::protobuf::Value val;
             val.set_string_value("🐑 " + std::to_string(i));
             return val;
           });
  }
  return {};
}
