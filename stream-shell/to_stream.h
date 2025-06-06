#pragma once

#include "operand.h"
#include "scope.h"
#include "stream_parser.h"

/**
 * Used to flatten Operands into a Stream.
 */
struct ToStream {
  ToStream(const Env &env, const Scope &scope) : _env{env}, _scope{scope} {}

  auto operator()(const IsValue auto &value) const -> Stream { return ranges::yield(value); }
  auto operator()(const google::protobuf::Value &value) const -> Stream {
    if (value.has_list_value()) {
      auto list = value.list_value();
      return ranges::views::iota(0, list.values().size()) |
             ranges::views::transform([list](auto i) { return list.values(i); });
    }
    return ranges::yield(value);
  }
  auto operator()(const Stream &stream) const { return stream; }
  auto operator()(const StreamRef &ref) const -> Stream {
    if (auto it = _scope.env_overrides.find(ref.name); it != _scope.env_overrides.end()) {
      return it->second({});
    } else if (auto stream = _env.getEnv(ref)) {
      return stream({});
    }
    return ranges::yield(std::unexpected(Error::kInvalidStreamRef));
  }
  auto operator()(const Word &word) const -> Stream {
    google::protobuf::Value value;
    value.set_string_value(Token(word.value) | ranges::to<std::string>);
    return ranges::yield(std::move(value));
  }

  auto operator()(const Operand &operand) const -> Stream { return std::visit(*this, operand); }

  const Env &_env;
  const Scope &_scope;
};
