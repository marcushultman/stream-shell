#pragma once

#include <google/protobuf/util/json_util.h>
#include "operand.h"
#include "scope.h"
#include "stream-shell/lift.h"
#include "stream_parser.h"

using namespace std::string_literals;

/**
 * Used for:
 * 1. JSON parsing (serialize Operands + join + parse) (no scope vars)
 * 2. String interpolation of StreamRef (including scope vars)
 * 3. Print Value in REPL.
 */
struct ToString final {
  ToString() = delete;

  using Result = Result<std::string>;

  struct Value {
    auto operator()(const google::protobuf::BytesValue &val) const -> Result {
      // todo: figure out when/if to encode
      return val.value();
    }
    auto operator()(const google::protobuf::Message &val) const -> Result {
      std::string str;
      (void)google::protobuf::util::MessageToJsonString(val, &str).ok();
      return str;
    }
    auto operator()(const google::protobuf::Value &val) const -> Result {
      if (val.has_string_value()) {
        return val.string_value();
      }
      return (*this)(static_cast<const google::protobuf::Message &>(val));
    }

    auto operator()(const ::Value &value) const -> Result { return std::visit(*this, value); }
  };

  struct Operand : Value {
    Operand(const Env &env, Scope scope, bool escape_var = false)
        : _env{env}, _scope{std::move(scope)}, _escape_var{escape_var} {}

    using Value::operator();

    auto operator()(auto *self, Stream stream) const -> Result {
      return lift(stream)
          .and_then([&](auto &&stream) {
            return lift(stream | ranges::views::transform([&](auto &&value) {
                          return std::visit(*self, std::move(value));
                        }));
          })
          .transform(
              [](auto &&s) { return s | ranges::views::join(" ") | ranges::to<std::string>; });
    }
    auto operator()(auto *self, const StreamRef &ref) const -> Result {
      if (auto it = _scope.vars.find(ref.name); _escape_var && it != _scope.vars.end()) {
        return std::visit(*self, *it->second);
      } else if (auto it = _scope.env_overrides.find(ref.name); it != _scope.env_overrides.end()) {
        return (*self)(it->second({}));
      } else if (auto stream = _env.getEnv(ref)) {
        return (*self)(stream({}));
      }
      return std::unexpected(Error::kInvalidStreamRef);
    }
    auto operator()(const Word &word) const -> Result {
      return Token(word.value) | ranges::to<std::string>;
    }

    //
    auto operator()(const StreamRef &ref) const -> Result { return (*this)(this, ref); }
    auto operator()(Stream stream) const -> Result { return (*this)(this, stream); }

    auto operator()(const ::Operand &operand) const -> Result { return std::visit(*this, operand); }

    const Env &_env;
    const Scope _scope;
    bool _escape_var;
  };
};
