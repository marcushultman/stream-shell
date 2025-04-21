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
  auto operator()(const auto &lhs, const auto &rhs) {
    return isTruthy(lhs) ? Expr::result_type(rhs) : std::unexpected(Error::kCoalesceSkip);
  }
  auto operator()(Error err, const auto &) { return std::unexpected(err); }
};

struct TernaryEvaluation {
  auto operator()(const auto &lhs, const auto &) -> Expr::result_type { return lhs; }
  auto operator()(Error err, const auto &rhs) -> Expr::result_type {
    return err == Error::kCoalesceSkip ? Expr::result_type(rhs) : std::unexpected(err);
  }
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
  auto operator()(const auto &) {
    // todo: implement backgrounding
    return std::unexpected(Error::kInvalidOp);
  }
};

/**
 * Visits Operand variants transforming Values. For use in operators and builins
 */
template <typename ValueT>
struct ValueTransform {
  ValueTransform(ValueT &&value_t) : _value_t(std::move(value_t)) {}

  auto operator()(const IsExprValue auto &...v) -> Operand {
    if (auto result = _value_t(v...)) {
      return std::visit([](auto value) -> Operand { return value; }, *result);
    } else {
      return [result](auto &) { return result; };
    }
  }
  auto operator()(const auto &...v) -> Operand { return eval(v...); }

 private:
  auto eval(const Expr &v) -> Expr {
    return [value_t = _value_t, v](const Closure &closure) mutable {
      return v(closure).and_then(
          [&](auto v) { return std::visit([&](auto v) { return value_t(v); }, v); });
    };
  }

  auto eval(const Expr &lhs, const IsValue auto &rhs) -> Expr {
    return [lhs, value_t = _value_t, rhs](const Closure &closure) mutable {
      return lhs(closure)
          .and_then([&](auto lhs) {
            return std::visit([&](IsExprValue auto lhs) { return value_t(lhs, rhs); }, lhs);
          })
          .or_else([&](Error err) { return value_t(err, rhs); });
    };
  }
  auto eval(const IsValue auto &lhs, const Expr &rhs) -> Expr {
    return [lhs, value_t = _value_t, rhs](const Closure &closure) mutable {
      return rhs(closure).and_then(
          [&](auto rhs) { return std::visit([&](auto &rhs) { return value_t(lhs, rhs); }, rhs); });
    };
  }
  auto eval(const Expr &lhs, const Expr &rhs) -> Expr {
    return [lhs, op = *this, rhs](const Closure &closure) mutable {
      return rhs(closure).and_then([&](auto rhs) {
        return std::visit([&](auto rhs) { return op.eval(lhs, rhs)(closure); }, rhs);
      });
    };
  }
  auto eval(const auto &...) -> Expr {
    return [](auto &) { return std::unexpected(Error::kInvalidOp); };
  }

  ValueT _value_t;
};

struct OperandOp {
  OperandOp(Token op) : op{op} {}

  auto operator()(const auto &v) const -> Operand {
    return ValueTransform([&](const auto &v) -> Expr::result_type {
      if (op == "+") return ValueOp<std::identity>()(v);
      if (op == "-") return ValueOp<std::negate<>>()(v);
      if (op == "!") return ValueOp<std::logical_not<>, bool>()(v);
      if (op == "..") return ValueOp<Iota>()(v);
      if (op == "&") return Background()(v);
      return std::unexpected(Error::kInvalidOp);
    })(v);
  }
  auto operator()(const auto &...v) const -> Operand {
    return ValueTransform([op = op](const auto &...v) -> Expr::result_type {
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
      return std::unexpected(Error::kInvalidOp);
    })(v...);
  }

 private:
  Token op;
};
