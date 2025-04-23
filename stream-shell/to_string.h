#pragma once

#include <google/protobuf/util/json_util.h>
#include "closure.h"
#include "operand.h"
#include "stream-shell/lift.h"
#include "stream_parser.h"

using namespace std::string_literals;

/**
 * Used to serialized external command args and interpolating string literals.
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
  };

  struct Operand : Value {
    Operand(const Env &env, const Closure &closure, bool escape_var = false)
        : _env{env}, _closure{closure}, _escape_var{escape_var} {}

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
      if (auto it = _closure.vars.find(ref.name); _escape_var && it != _closure.vars.end()) {
        return std::visit(*self, *it->second);
      } else if (auto it = _closure.env_overrides.find(ref.name);
                 it != _closure.env_overrides.end()) {
        return (*self)(it->second());
      } else if (auto stream = _env.getEnv(ref)) {
        return (*self)(stream());
      }
      return std::unexpected(Error::kInvalidStreamRef);
    }
    auto operator()(const Word &word) const -> Result {
      return Token(word.value) | ranges::to<std::string>;
    }
    auto operator()(auto *self, const Expr &value) const -> Result {
      if (auto result = value(_closure)) {
        return std::visit(*self, std::move(*result));
      } else {
        return std::unexpected(result.error());
      }
    }

    //
    auto operator()(const StreamRef &ref) const -> Result { return (*this)(this, ref); }
    auto operator()(Stream stream) const -> Result { return (*this)(this, stream); }
    auto operator()(const Expr &value) const -> Result { return (*this)(this, value); }

    auto operator()(const ::Operand &operand) const -> Result { return std::visit(*this, operand); }

    const Env &_env;
    const Closure &_closure;
    bool _escape_var;
  };
};
