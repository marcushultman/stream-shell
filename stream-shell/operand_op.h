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

template <typename T>
concept IsValue = InVariant<ClosureValue::result_type::value_type, T>;

struct OperandOp {
  OperandOp(std::string_view op) : op{op} {}

  // todo: implement ops for StreamRef, (binary) Word

  auto operator()(const IsValue auto &...v) const -> Operand {
    if (auto result = eval(v...)) {
      return std::visit([](auto value) -> Operand { return value; }, *result);
    } else {
      return [result](auto &) { return result; };
    }
  }
  auto operator()(const auto &...v) const -> Operand { return eval(v...); }

 private:
  auto eval(const IsValue auto &v) const -> ClosureValue::result_type {
    if (op == "+") return ValueOp<std::identity>()(v);
    if (op == "-") return ValueOp<std::negate<>>()(v);
    if (op == "!") return ValueOp<std::logical_not<>, bool>()(v);
    if (op == "..") return ValueOp<Iota>()(v);
    return std::unexpected(Error::kInvalidOp);
  }
  auto eval(const ClosureValue &v) const -> ClosureValue {
    return [op = *this, v](const Closure &closure) {
      return v(closure).and_then(
          [&](auto v) { return std::visit([&](IsValue auto v) { return op.eval(v); }, v); });
    };
  }

  auto eval(const IsValue auto &lhs, const IsValue auto &rhs) const -> ClosureValue::result_type {
    if (op == "||") return ValueOp<std::logical_or<>, bool>()(lhs, rhs);
    if (op == "&&") return ValueOp<std::logical_and<>, bool>()(lhs, rhs);
    if (op == "==") return ValueOp<std::equal_to<>, bool>()(lhs, rhs);
    if (op == "!=") return ValueOp<std::not_equal_to<>, bool>()(lhs, rhs);
    if (op == "<") return ValueOp<std::less<>, bool>()(lhs, rhs);
    if (op == "<=") return ValueOp<std::less_equal<>, bool>()(lhs, rhs);
    if (op == ">") return ValueOp<std::greater<>, bool>()(lhs, rhs);
    if (op == ">=") return ValueOp<std::greater_equal<>, bool>()(lhs, rhs);
    if (op == "+") return ValueOp<std::plus<>>()(lhs, rhs);
    if (op == "-") return ValueOp<std::minus<>>()(lhs, rhs);
    if (op == "*") return ValueOp<std::multiplies<>>()(lhs, rhs);
    if (op == "/") return ValueOp<std::divides<>>()(lhs, rhs);
    if (op == "%") return ValueOp<std::modulus<>>()(lhs, rhs);
    if (op == "?") {
      return isTruthy(lhs) ? ClosureValue::result_type(rhs) : std::unexpected(Error::kCoalesceSkip);
    }
    if (op == "?:") return lhs;
    if (op == "..") return ValueOp<Iota>()(lhs, rhs);
    return std::unexpected(Error::kInvalidOp);
  }

  auto eval(const ClosureValue &lhs, const IsValue auto &rhs) const -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) {
      return lhs(closure)
          .and_then([&](auto lhs) {
            return std::visit([&](IsValue auto lhs) { return op.eval(lhs, rhs); }, lhs);
          })
          .or_else([&](Error err) -> ClosureValue::result_type {
            if (op.op == "?:" && err == Error::kCoalesceSkip) {
              return rhs;
            }
            return std::unexpected(err);
          });
    };
  }
  auto eval(const IsValue auto &lhs, const ClosureValue &rhs) const -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) {
      return rhs(closure).and_then([&](auto rhs) {
        return std::visit([&](IsValue auto rhs) { return op.eval(lhs, rhs); }, rhs);
      });
    };
  }
  auto eval(const ClosureValue &lhs, const ClosureValue &rhs) const -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) {
      return rhs(closure).and_then([&](auto rhs) {
        return std::visit([&](IsValue auto rhs) { return op.eval(lhs, rhs)(closure); }, rhs);
      });
    };
  }
  auto eval(const auto &...) const -> ClosureValue {
    return [](auto &) { return std::unexpected(Error::kInvalidOp); };
  }

  std::string_view op;
};
