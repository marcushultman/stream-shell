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
inline std::optional<Stream> runBuiltin(Word cmd,
                                        Env &env,
                                        const Closure &closure,
                                        IsExprValue auto &&input,
                                        ranges::bidirectional_range auto args) {
  if (cmd.value == "add"sv) {
    return add(ToStream(env, closure), std::move(input), args);
  } else if (cmd.value == "get"sv) {
    return get(ToStream(env, closure), std::move(input), args);
  } else if (cmd.value == "now"sv) {
    return now(env);
  } else if (cmd.value == "prepend"sv) {
    return prepend(ToStream(env, closure), std::move(input), args);
  } else if (cmd.value == "exit"sv) {
    return ranges::views::generate([] { return exit(0), Value(); });
  }
  return {};
}
