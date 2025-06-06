#pragma once

#include <optional>
#include <string_view>
#include <range/v3/all.hpp>
#include "builtins/add.h"
#include "builtins/get.h"
#include "builtins/now.h"
#include "builtins/prepend.h"
#include "scope.h"
#include "stream-shell/stream_transform.h"
#include "to_stream.h"

using namespace std::string_view_literals;

inline std::optional<Stream> runBuiltin(
    Word cmd, Env &env, const Scope &scope, Stream &&input, ranges::bidirectional_range auto args) {
  if (cmd.value == "add"sv) {
    return std::move(input) | for_each([args](auto value) { return add(std::move(value), args); });
  } else if (cmd.value == "get"sv) {
    return std::move(input) | for_each([args](auto value) { return get(std::move(value), args); });
  } else if (cmd.value == "now"sv) {
    return now(env);
  } else if (cmd.value == "prepend"sv) {
    return prepend(ToStream(env, scope), std::move(input), args);
  } else if (cmd.value == "exit"sv) {
    return ranges::views::generate([] { return exit(0), Value(); });
  }
  return {};
}
