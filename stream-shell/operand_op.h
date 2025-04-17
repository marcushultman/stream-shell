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
  return std::visit([](auto &value) { return isTruthy(value); }, value);
}
inline bool isTruthy(const Stream &stream) {
  return ranges::all_of(Stream(stream), [](auto value) { return value && isTruthy(*value); });
}

struct TernaryCondition {
  auto operator()(const IsValue auto &lhs, const IsValue auto &rhs) {
    return isTruthy(lhs) ? ClosureValue::result_type(rhs) : std::unexpected(Error::kCoalesceSkip);
  }
};
struct TernaryTruthy {
  auto operator()(const IsValue auto &lhs, const IsValue auto &rhs) { return lhs; }
};

struct Iota {
  auto operator()(int64_t from) { return ranges::views::iota(from) | toNumber; }
  auto operator()(int64_t from, int64_t to) { return ranges::views::iota(from, to + 1) | toNumber; }

 private:
  static constexpr auto toNumber = ranges::views::transform([](auto i) {
    google::protobuf::Value value;
    value.set_number_value(i);
    return value;
  });
};

struct Background {
  auto operator()(Stream s) {
    // todo: implement backgrounding
    return std::unexpected(Error::kInvalidOp);
  }
};

/**
 * Visits Operand variants transforming Values. For use in operators and builins
 */
template <typename ValueT, typename StreamT = std::nullptr_t, typename ErrorT = std::nullptr_t>
struct ValueTransform {
  ValueTransform(ValueT &&t) : _value_t(std::move(t)) {}

  ValueTransform(ValueT &&value_t, StreamT &&stream_t, ErrorT &&error_t)
      : _value_t(std::move(value_t)),
        _stream_t(std::move(stream_t)),
        _error_t(std::move(error_t)) {}

  auto operator()(const ValueOrStream auto &...v) -> Operand {
    if (auto result = eval1(v...)) {
      return std::visit([](auto value) -> Operand { return value; }, *result);
    } else {
      return [result](auto &) { return result; };
    }
  }
  auto operator()(const auto &...v) -> Operand { return eval2(v...); }

 private:
  auto eval1(const Stream &v) -> ClosureValue::result_type { return _stream_t(v); }
  auto eval1(const IsValue auto &v) { return _value_t(v); }
  auto eval1(const IsValue auto &lhs, const IsValue auto &rhs) { return _value_t(lhs, rhs); }
  auto eval1(const ValueOrStream auto &...) -> ClosureValue::result_type {
    return std::unexpected(Error::kInvalidOp);
  }

  auto eval2(const ClosureValue &v) -> ClosureValue {
    return [op = *this, v](const Closure &closure) mutable {
      return v(closure).and_then(
          [&](auto v) { return std::visit([&](auto v) { return op.eval1(v); }, v); });
    };
  }

  auto eval2(const ClosureValue &lhs, const IsValue auto &rhs) -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) mutable {
      return lhs(closure)
          .and_then([&](auto lhs) {
            return std::visit([&](ValueOrStream auto lhs) { return op.eval1(lhs, rhs); }, lhs);
          })
          .or_else([&](Error err) { return op._error_t(err, rhs); });
    };
  }
  auto eval2(const IsValue auto &lhs, const ClosureValue &rhs) -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) mutable {
      return rhs(closure).and_then(
          [&](auto rhs) { return std::visit([&](auto &rhs) { return op.eval1(lhs, rhs); }, rhs); });
    };
  }
  auto eval2(const ClosureValue &lhs, const ClosureValue &rhs) -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) mutable {
      return rhs(closure).and_then([&](auto rhs) {
        return std::visit([&](auto rhs) { return op.eval2(lhs, rhs)(closure); }, rhs);
      });
    };
  }
  auto eval2(const auto &...) -> ClosureValue {
    return [](auto &) { return std::unexpected(Error::kInvalidOp); };
  }

  ValueT _value_t;
  StreamT _stream_t = {};
  ErrorT _error_t = {};
};

struct OperandOp {
  OperandOp(Token op) : op{op} {}

  auto operator()(const auto &v) const -> Operand {
    return ValueTransform(
        [&](const IsValue auto &v) -> ClosureValue::result_type {
          if (op == "+") return ValueOp<std::identity>()(v);
          if (op == "-") return ValueOp<std::negate<>>()(v);
          if (op == "!") return ValueOp<std::logical_not<>, bool>()(v);
          if (op == "..") return ValueOp<Iota>()(v);
          return std::unexpected(Error::kInvalidOp);
        },
        [&](const Stream &s) -> ClosureValue::result_type {
          if (op == "&") return Background()(s);
          return std::unexpected(Error::kInvalidOp);
        },
        [](auto err, auto &) { return std::unexpected(err); })(v);
  }
  auto operator()(const auto &...v) const -> Operand {
    return ValueTransform(
        [op = op](const IsValue auto &...v) -> ClosureValue::result_type {
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
          if (op == "?:") return TernaryTruthy()(v...);
          if (op == "..") return ValueOp<Iota>()(v...);
          return std::unexpected(Error::kInvalidOp);
        },
        {},
        [op = op](Error err, const IsValue auto &rhs) -> ClosureValue::result_type {
          if (op == "?:" && err == Error::kCoalesceSkip) {
            return rhs;
          }
          return std::unexpected(err);
        })(v...);
  }

 private:
  Token op;
};
