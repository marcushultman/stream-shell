#pragma once

#include "closure.h"
#include "operand.h"
#include "stream_parser.h"

struct ToStream {
  ToStream(const Env &env, const Closure &closure) : _env{env}, _closure{closure} {}

  auto operator()(Value val) const -> Stream { return ranges::views::single(std::move(val)); }
  auto operator()(google::protobuf::Value val) const -> Stream {
    if (val.has_list_value()) {
      auto size = val.list_value().values().size();
      return ranges::views::iota(0, size) |
             ranges::views::transform(
                 [list = std::move(*val.mutable_list_value())](auto i) { return list.values(i); });
    }
    return (*this)(Value(val));
  }
  auto operator()(Stream stream) const { return stream; }
  auto operator()(const StreamRef &ref) const -> Stream {
    if (auto it = _closure.env_overrides.find(ref.name); it != _closure.env_overrides.end()) {
      return it->second();
    } else if (auto stream = _env.getEnv(ref)) {
      return stream();
    }
    return ranges::views::single(std::unexpected(Error::kInvalidStreamRef));
  }
  auto operator()(const Word &word) const -> Stream {
    google::protobuf::Value value;
    value.set_string_value(Token(word.value) | ranges::to<std::string>);
    return ranges::views::single(std::move(value));
  }
  auto operator()(const Expr &value) const -> Stream {
    if (auto result = value(_closure)) {
      return std::visit(*this, std::move(*result));
    } else {
      return ranges::views::single(std::unexpected(result.error()));
    }
  }

  const Env &_env;
  const Closure &_closure;
};
