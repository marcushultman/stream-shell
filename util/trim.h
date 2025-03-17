#pragma once

#include <string_view>

// util/trim.h

inline std::string_view trimLeft(std::string_view str) {
  size_t start = str.find_first_not_of(" \t\n\r\f\v");
  return (start == std::string_view::npos) ? "" : str.substr(start);
}

inline std::string_view trimRight(std::string_view str) {
  size_t end = str.find_last_not_of(" \t\n\r\f\v");
  return (end == std::string_view::npos) ? "" : str.substr(0, end + 1);
}

inline std::string_view trim(std::string_view str) {
  return trimLeft(trimRight(str));
}
