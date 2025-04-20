#pragma once

#include <expected>
#include <functional>
#include <string_view>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <range/v3/all.hpp>

//

// todo: ranges::views::split is forward. std is bidirectional, but join_with is not implemented
using Token = ranges::any_view<const char, ranges::category::forward>;

inline auto operator<(Token lhs, Token rhs) {
  return ranges::lexicographical_compare(lhs, rhs);
}

inline auto operator==(std::ranges::range auto lhs, std::ranges::range auto rhs) {
  return ranges::equal(lhs, rhs);
}
inline auto operator==(std::ranges::range auto lhs, const char *rhs) {
  return ranges::equal(lhs, std::string_view(rhs));
}

inline auto operator!=(std::ranges::range auto lhs, const char *rhs) {
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

  kCoalesceSkip,

  kMissingOperator,
  kMissingOperand,
  kMissingVariable,

  kExecError,
  kExecPipeError,
  kExecForkError,

  kInvalidNumberOp,
  kInvalidBoolOp,
  kInvalidStringOp,
  kInvalidOp,

  kInvalidClosureSignature,
  kInvalidStreamRef,
};

template <typename T>
using Result = std::expected<T, Error>;

using Stream = ranges::any_view<Result<Value>>;
using StreamFactory = std::function<Stream()>;

struct StreamRef {
  Token name;
  auto operator<(const StreamRef &rhs) const { return name < rhs.name; }
};

struct Env {
  virtual ~Env() = default;
  virtual StreamFactory getEnv(StreamRef) const = 0;
  virtual void setEnv(StreamRef, StreamFactory) = 0;
  virtual bool sleepUntil(std::chrono::steady_clock::time_point) = 0;
};

struct Print {
  struct Pull {
    bool full = false;
    bool operator==(const Pull &) const = default;
  };
  struct Slice {
    size_t window = 0;
    bool operator==(const Slice &) const = default;
  };
  struct WriteFile {
    ranges::any_view<const char, ranges::category::forward> filename;
    bool operator==(const WriteFile &) const = default;
  };
  using Mode = std::variant<Pull, Slice, WriteFile>;
};

using PrintableStream = std::pair<Stream, Print::Mode>;

struct StreamParser {
  virtual ~StreamParser() = default;
  virtual auto parse(
      ranges::any_view<ranges::any_view<const char, ranges::category::bidirectional>>)
      -> PrintableStream = 0;
};

/**
 * Implements Shunting Yard for tokenized stream-shell input.
 */
std::unique_ptr<StreamParser> makeStreamParser(Env &env);
