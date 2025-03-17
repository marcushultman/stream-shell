#pragma once

#include <optional>
#include <string_view>
#include <range/v3/all.hpp>
#include "value.h"

std::optional<ranges::any_view<Value>> findBuiltin(std::string_view cmd) {
  if (cmd.starts_with("iota")) {
    return ranges::views::iota(1) |
           ranges::views::transform([](auto i) { return Value("sheep " + std::to_string(i)); });
  }
  return {};
}
