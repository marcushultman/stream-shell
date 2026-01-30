#pragma once

#include <expected>
#include <functional>
#include <string_view>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <range/v3/all.hpp>
#include "variant_ext.h"

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

template <ranges::category C>
inline auto operator==(ranges::any_view<const char, C> lhs, const char *rhs) {
  return ranges::equal(lhs, std::string_view(rhs));
}

template <ranges::category C>
inline auto operator!=(ranges::any_view<const char, C> lhs, const char *rhs) {
  return !(lhs == rhs);
}

//

using Value = std::variant<google::protobuf::BytesValue,  // Bytes
                           google::protobuf::Value,       // Primitives & JSON
                           google::protobuf::Any>;        // Strong types

enum class Error : int {
  kSuccess = 0,
  kUnknown = 1,
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
  kInvalidOp,

  kConfigError,

  kInvalidClosureSignature,
  kInvalidStreamRef,
};

template <typename T>
using Result = std::expected<T, Error>;

using Stream = ranges::any_view<Result<Value>>;
using StreamFactory = std::function<Stream(Stream)>;

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
  virtual StreamFactory getEnv(StreamRef) const = 0;
  virtual void setEnv(StreamRef, StreamFactory) = 0;
  virtual bool sleepUntil(std::chrono::steady_clock::time_point) = 0;
  virtual ssize_t read(int fd, google::protobuf::BytesValue &bytes) = 0;
};

struct StreamParser {
  virtual ~StreamParser() = default;
  virtual auto parse(
      ranges::any_view<ranges::any_view<const char, ranges::category::bidirectional>>)
      -> Stream = 0;
};

/**
 * Implements Shunting Yard for tokenized stream-shell input.
 */
std::unique_ptr<StreamParser> makeStreamParser(Env &env);
