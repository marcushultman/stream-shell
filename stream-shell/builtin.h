#pragma once

#include <cstdlib>
#include <optional>
#include <string_view>
#include <range/v3/all.hpp>
#include "builtins/add.h"
#include "builtins/args.h"
#include "builtins/echo.h"
#include "builtins/get.h"
#include "builtins/now.h"
#include "stream-shell/stream_transform.h"

using namespace std::string_view_literals;

inline std::optional<Stream> runBuiltin(std::string_view cmd,
                                        const google::protobuf::Struct &config,
                                        Stream input,
                                        Env &env) {
  if (cmd == "args"sv) {
    return args(config);

  } else if (cmd == "add"sv) {
    return std::move(input) | for_each([=](auto val) { return add(std::move(val), config); });

  } else if (cmd == "echo"sv) {
    return echo(config);

  } else if (cmd == "get"sv) {
    return std::move(input) | for_each([=](auto val) { return get(std::move(val), config); });

  } else if (cmd == "now"sv) {
    return now(env);

  } else if (cmd == "exit"sv) {
    return ranges::views::generate([]() -> Value { std::exit(0); });
  }
  return {};
}
