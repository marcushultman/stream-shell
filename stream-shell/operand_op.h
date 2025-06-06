#pragma once

#include <type_traits>
#include "closure.h"
#include "operand.h"
#include "stream_parser.h"
#include "value_op.h"

inline bool isTruthy(const google::protobuf::BytesValue &bytes) {
  return ranges::any_of(bytes.value(), std::identity());
}
inline bool isTruthy(const google::protobuf::Value &value) {
  if (value.has_null_value()) {
    return false;
  } else if (value.has_number_value()) {
    return value.number_value() > 0;
  } else if (value.has_string_value()) {
    return !value.string_value().empty();
  } else if (value.has_bool_value()) {
    return value.bool_value();
  } else if (value.has_struct_value()) {
    return true;
  } else if (value.has_list_value()) {
    return !value.list_value().values().empty();
  }
  return false;
}
inline bool isTruthy(const google::protobuf::Any &value) {
  return true;
}
inline bool isTruthy(const Value &value) {
  return std::visit([](const auto &value) { return isTruthy(value); }, value);
}
inline bool isTruthy(Stream stream) {
  return ranges::all_of(stream, [](auto &&value) { return value && isTruthy(*value); });
}

struct TernaryCondition {
  auto operator()(const IsValue auto &lhs, const auto &rhs) -> Stream {
    return isTruthy(lhs) ? Stream(ranges::yield(rhs))
                         : ranges::yield(std::unexpected(Error::kCoalesceSkip));
  }
  auto operator()(Error err, const auto &) -> Stream { return ranges::yield(std::unexpected(err)); }
};

struct TernaryEvaluation {
  auto operator()(const IsValue auto &lhs, const IsValue auto &) -> Stream {
    return ranges::yield(lhs);
  }
  auto operator()(Error err, const IsValue auto &rhs) -> Stream {
    return err == Error::kCoalesceSkip ? Stream(ranges::yield(rhs))
                                       : ranges::yield(std::unexpected(err));
  }
};

struct Iota {
  auto operator()(int64_t from) -> Stream { return ranges::views::iota(from) | toNumber; }
  auto operator()(int64_t from, int64_t to) -> Stream {
    return ranges::views::iota(from, to + 1) | toNumber;
  }

 private:
  static constexpr auto toNumber = ranges::views::transform([](auto i) {
    google::protobuf::Value value;
    value.set_number_value(i);
    return value;
  });
};

struct Background {
  auto operator()(const auto &) -> Stream {
    // todo: implement backgrounding
    return ranges::yield(std::unexpected(Error::kInvalidOp));
  }
};

/**
 * Visits Operand variants transforming Values. For use in operators and builins
 */
template <typename ValueT>
struct ValueTransform {
  ValueTransform(ValueT &&value_t) : _value_t(std::move(value_t)) {}

  auto operator()(const IsValue auto &...v) -> Stream { return _value_t(v...); }
  auto operator()(const auto &...v) -> Stream { return eval(v...); }

 private:
  auto eval(const Stream &s) -> Stream {
    return Stream(s) | ranges::views::for_each([value_t = _value_t](const Result<Value> &result) {
             return result
                 .transform([&](const Value &value) {
                   return std::visit([&](const auto &value) { return value_t(value); }, value);
                 })
                 .or_else([](Error err) -> Result<Stream> {
                   return ranges::yield(std::unexpected(err));
                 })
                 .value();
           });
  }

  auto eval(const Result<Value> &lhs, const Result<Value> &rhs) -> Stream {
    return rhs
        .and_then([&](const Value &rhs) {
          return lhs
              .transform([&](const Value &lhs) {
                return std::visit(
                    [&](const auto &lhs, const auto &rhs) { return _value_t(lhs, rhs); }, lhs, rhs);
              })
              .or_else([&](Error err) -> Result<Stream> {
                return std::visit([&](const auto &rhs) { return _value_t(err, rhs); }, rhs);
              });
        })
        .or_else([](Error err) -> Result<Stream> { return ranges::yield(std::unexpected(err)); })
        .value();
  }
  auto eval(const Stream &lhs, const IsValue auto &rhs) -> Stream {
    return eval(lhs, Stream(ranges::yield(rhs)));
  }
  auto eval(const IsValue auto &lhs, const Stream &rhs) -> Stream {
    return eval(Stream(ranges::yield(lhs)), rhs);
  }
  auto eval(const Stream &lhs, const Stream &rhs) -> Stream {
    return ranges::views::zip(Stream(lhs), Stream(rhs)) |
           ranges::views::for_each([op = *this](const auto &results) mutable -> Stream {
             auto &[lhs, rhs] = results;
             return op.eval(lhs, rhs);
           });
  }
  auto eval(const auto &...) -> Stream { return ranges::yield(std::unexpected(Error::kInvalidOp)); }

  ValueT _value_t;
};

struct OperandOp {
  OperandOp(Token op) : op{op} {}

  auto operator()(const auto &v) const -> Operand {
    return ValueTransform([&](const auto &v) -> Stream {
      if (op == "+") return ValueOp<std::identity>()(v);
      if (op == "-") return ValueOp<std::negate<>>()(v);
      if (op == "!") return ValueOp<std::logical_not<>, bool>()(v);
      if (op == "..") return ValueOp<Iota>()(v);
      if (op == "&") return Background()(v);
      return ranges::yield(std::unexpected(Error::kInvalidOp));
    })(v);
  }
  auto operator()(const auto &...v) const -> Operand {
    return ValueTransform([op = op](const auto &...v) -> Stream {
      if (op == "||") return ValueOp<std::logical_or<>, bool>()(v...);
      if (op == "&&") return ValueOp<std::logical_and<>, bool>()(v...);
      if (op == "==") return ValueOp<std::equal_to<>, bool>()(v...);
      if (op == "!=") return ValueOp<std::not_equal_to<>, bool>()(v...);
      if (op == "<") return ValueOp<std::less<>, bool>()(v...);
      if (op == "<=") return ValueOp<std::less_equal<>, bool>()(v...);
      if (op == ">") return ValueOp<std::greater<>, bool>()(v...);
      if (op == ">=") return ValueOp<std::greater_equal<>, bool>()(v...);
      if (op == "+") return ValueOp<std::plus<>>()(v...);
      if (op == "-") return ValueOp<std::minus<>>()(v...);
      if (op == "*") return ValueOp<std::multiplies<>>()(v...);
      if (op == "/") return ValueOp<std::divides<>>()(v...);
      if (op == "%") return ValueOp<std::modulus<>>()(v...);
      if (op == "?") return TernaryCondition()(v...);
      if (op == "..") return ValueOp<Iota>()(v...);
      return ranges::yield(std::unexpected(Error::kInvalidOp));
    })(v...);
  }

 private:
  Token op;
};
