#pragma once

#include <expected>
#include <functional>
#include <string_view>
#include <utility>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <range/v3/all.hpp>
#include "process.h"

//

// todo: ranges::views::split is forward. std is bidirectional, but join_with is not implemented
using Token = ranges::any_view<const char, ranges::category::forward>;

inline auto operator<(Token lhs, Token rhs) {
  return ranges::lexicographical_compare(lhs, rhs);
}

inline auto operator==(std::ranges::range auto lhs, std::ranges::range auto rhs) {
  return ranges::equal(lhs, rhs);
}
inline auto operator!=(std::ranges::range auto lhs, std::ranges::range auto rhs) {
  return !(lhs == rhs);
}

namespace ranges {

inline auto operator<(std::string_view lhs, range auto rhs) {
  return lexicographical_compare(lhs, rhs);
}

inline auto operator<(range auto lhs, std::string_view rhs) {
  return lexicographical_compare(lhs, rhs);
}

inline auto operator==(range auto lhs, std::string_view rhs) {
  return equal(lhs, rhs);
}

inline auto operator!=(range auto lhs, std::string_view rhs) {
  return !(lhs == rhs);
}

}  // namespace ranges

//

using Value = std::variant<google::protobuf::BytesValue,  // Bytes
                           google::protobuf::Value,       // Primitives & JSON
                           google::protobuf::Any>;        // Strong types

enum class Error : int {
  kSuccess = 0,
  kUnknown,
  kAborted,

  kParseError,
  kJsonError,

  kMissingOperator,
  kMissingOperand,
  kMissingTernary,

  kExecError,
  kExecPipeError,
  kExecForkError,
  kExecNonZeroStatus,
  kExecReadError,

  kInvalidNumberOp,
  kInvalidBoolOp,
  kInvalidStringOp,
  kInvalidStructOp,
  kInvalidOp,

  kConfigError,

  kInvalidClosureSignature,
  kInvalidStreamRef,
};

template <typename T>
using Result = std::expected<T, Error>;

using Stream = ranges::any_view<Result<Value>>;

using EnvStream = std::function<Stream()>;
using Closure = std::function<Stream(Value)>;

struct StreamRef {
  std::string name;

  auto operator<=>(const StreamRef &) const = default;

  StreamRef static fromToken(Token token) {
    using namespace std::string_view_literals;
    assert(ranges::starts_with(token, "$"sv));
    return {.name = token | ranges::views::drop(1) | ranges::to<std::string>};
  }
};

struct Env {
  virtual ~Env() = default;
  virtual EnvStream getEnv(StreamRef) const = 0;
  virtual void setEnv(StreamRef, EnvStream) = 0;
  virtual bool blockUntilChange(StreamRef) = 0;

  virtual bool sleepUntil(std::chrono::steady_clock::time_point) = 0;
  virtual ssize_t read(int fd, google::protobuf::BytesValue &bytes) = 0;
  virtual Stream exit() = 0;
};

struct StreamParser {
  virtual ~StreamParser() = default;

  using Tokens = ranges::any_view<ranges::any_view<const char, ranges::category::bidirectional>>;

  virtual auto parse(Tokens) -> Stream = 0;
  virtual auto parseInteractive(Tokens) -> void = 0;
};

/**
 * Implements Shunting Yard for tokenized stream-shell input.
 */
std::unique_ptr<StreamParser> makeStreamParser(Env &env);
