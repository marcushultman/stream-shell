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
#include "builtins/observe.h"
#include "stream-shell/command.h"
#include "stream-shell/stream_transform.h"

using namespace std::string_view_literals;

inline std::unique_ptr<Command> buildBuiltin(Env &env,
                                             std::string_view cmd,
                                             google::protobuf::Struct &config,
                                             std::unique_ptr<Command> &upstream) {
  if (cmd == "args"sv) {
    return std::make_unique<StreamConstantCommand>(args(config));

  } else if (cmd == "add"sv) {
    return std::make_unique<ClosureCommand>(
        env, std::move(upstream), [=](auto val) { return add(std::move(val), config); });

  } else if (cmd == "echo"sv) {
    return std::make_unique<StreamConstantCommand>(echo(config));

  } else if (cmd == "get"sv) {
    return std::make_unique<ClosureCommand>(
        env, std::move(upstream), [=](auto val) { return get(std::move(val), config); });

  } else if (cmd == "now"sv) {
    return std::make_unique<StreamConstantCommand>(now(env));

  } else if (cmd == "observe"sv) {
    return std::make_unique<StreamConstantCommand>(observe(env, config));

  } else if (cmd == "exit"sv) {
    return std::make_unique<StreamConstantCommand>(env.exit());
  }
  return {};
}
