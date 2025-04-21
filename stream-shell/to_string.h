#pragma once

#include <google/protobuf/util/json_util.h>
#include "closure.h"
#include "operand.h"
#include "stream_parser.h"

using namespace std::string_literals;

struct ToString {
  ToString(const Env &env, const Closure &closure, bool escape_var = false)
      : _env{env}, _closure{closure}, _escape_var{escape_var} {}

  auto operator()(google::protobuf::BytesValue val) const -> Result<std::string> {
    // todo: figure out when/if to encode
    return val.value();
  }
  auto operator()(const google::protobuf::Message &val) const -> Result<std::string> {
    std::string str;
    (void)google::protobuf::util::MessageToJsonString(val, &str).ok();
    return str;
  }
  auto operator()(Stream stream) const -> Result<std::string> {
    Result<std::string> err;
    auto str =
        stream | ranges::views::transform([&](auto &&result) {
          return (err = err.and_then([&](auto &) { return result; }).and_then([&](auto &&value) {
                   return std::visit(*this, value);
                 }))
              .value_or(""s);
        }) |
        ranges::views::join(" ") | ranges::to<std::string>;
    return err.transform([&](auto &) { return str; });
  }
  auto operator()(const StreamRef &ref) const -> Result<std::string> {
    if (auto it = _closure.vars.find(ref.name); _escape_var && it != _closure.vars.end()) {
      return std::visit(*this, *it->second);
    } else if (auto it = _closure.env_overrides.find(ref.name);
               it != _closure.env_overrides.end()) {
      return (*this)(it->second());
    } else if (auto stream = _env.getEnv(ref)) {
      return (*this)(stream());
    }
    return std::unexpected(Error::kInvalidStreamRef);
  }
  auto operator()(const Word &word) const -> Result<std::string> {
    return Token(word.value) | ranges::to<std::string>;
  }
  auto operator()(const Expr &value) const -> Result<std::string> {
    if (auto result = value(_closure)) {
      return std::visit(*this, std::move(*result));
    } else {
      return std::unexpected(result.error());
    }
  }

  const Env &_env;
  const Closure &_closure;
  bool _escape_var;
};
