#pragma once

#include <string_view>

#include <range/v3/all.hpp>

// util/to_string_view.h

inline auto toStringView(ranges::range auto &&s) -> std::string_view {
  return std::string_view(&*s.begin(), ranges::distance(s));
}
