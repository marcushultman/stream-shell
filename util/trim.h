#pragma once

#include <range/v3/all.hpp>

inline auto trim(ranges::bidirectional_range auto str) {
  auto is_space = [](auto c) { return std::isspace(c); };
  return std::forward<decltype(str)>(str) | ranges::views::drop_while(is_space) |
         ranges::views::reverse | ranges::views::drop_while(is_space) | ranges::views::reverse;
}

inline auto trim(ranges::bidirectional_range auto str, size_t leading, size_t trailing) {
  return std::forward<decltype(str)>(str) | ranges::views::drop(leading) | ranges::views::reverse |
         ranges::views::drop(trailing) | ranges::views::reverse;
}
