#pragma once

#include <string_view>
#include <range/v3/all.hpp>
#include "builtin.h"
#include "util/to_string_view.h"
#include "util/trim.h"

struct Command {
  Command(auto str) : _str{trim(std::string_view(str))} {}

  auto run(ranges::range auto stream) -> ranges::any_view<Value> {
    if (_str.starts_with('{') && _str.ends_with('}')) {
      return ranges::views::single(Value("todo: closure"));
    } else if (auto builtin = findBuiltin(_str)) {
      return *builtin;
    }
    // todo: parse parenthesis
    return _str | ranges::views::split_when([&](char c) { return ranges::contains(" \n", c); }) |
           ranges::views::transform([](auto &&s) { return Value(toStringView(s)); });
  }

 private:
  std::string_view _str;
};
