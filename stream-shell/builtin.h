#pragma once

#include <optional>
#include <string_view>
#include <range/v3/all.hpp>
#include "builtins/add.h"
#include "builtins/get.h"
#include "builtins/now.h"
#include "builtins/prepend.h"
#include "closure.h"
#include "to_stream.h"

using namespace std::string_view_literals;

// todo: pass args here and return Operand (since builtins may fail)
inline std::optional<Stream> runBuiltin(Env &env,
                                        const Closure &closure,
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
    return add(ToStream(env, closure), std::forward<decltype(input)>(input), args);
  } else if (ranges::starts_with(cmd.value, "get"sv)) {
    return get(ToStream(env, closure), std::forward<decltype(input)>(input), args);
  } else if (ranges::starts_with(cmd.value, "now"sv)) {
    return now(env);
  } else if (ranges::starts_with(cmd.value, "prepend"sv)) {
    return prepend(ToStream(env, closure), std::forward<decltype(input)>(input), args);
  }
  if (cmd.value == "exit") {
    return ranges::views::generate([] { return exit(0), Value(); });
  }
  return {};
}
