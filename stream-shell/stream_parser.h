#pragma once

#include <expected>
#include <string_view>
#include <vector>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <range/v3/all.hpp>

using Value = std::variant<google::protobuf::BytesValue,  // Bytes
                           google::protobuf::Value,       // Primitives & JSON
                           google::protobuf::Any>;        // Strong types

enum class Error : int {
  kSuccess = 0,
  kUnknown = 1,
  kParseError,
  kJsonError,

  kCoalesceSkip,

  kMissingOperand,
  kMissingVariable,

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
  std::string_view name;
};

struct Env {
  virtual ~Env() = default;
  virtual StreamFactory getEnv(StreamRef) const = 0;
  virtual void setEnv(StreamRef, StreamFactory) = 0;
};

struct Print {
  struct Pull {
    bool full = false;
  };
  struct Slice {
    size_t window = 0;
  };
  using Mode = std::variant<Pull, Slice>;
};

using PrintableStream = std::pair<Stream, Print::Mode>;

struct StreamParser {
  virtual ~StreamParser() = default;
  virtual auto parse(std::vector<std::string_view> &&) -> PrintableStream = 0;
};

/**
 * Implements Shunting Yard for tokenized stream-shell input.
 */
std::unique_ptr<StreamParser> makeStreamParser(Env &env);
