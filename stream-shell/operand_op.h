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

template <typename T>
concept IsValue = InVariant<Value, T>;

struct OperandOp {
  OperandOp(std::string_view op) : op{op} {}

  // FROM VALUE_OP

  // Operand operator()(const Stream &stream) {
  //   if constexpr (OpResult<Op, bool, bool>::value) {
  //     google::protobuf::Value result;
  //     result.set_bool_value(Op()(ranges::distance(Stream(stream))));
  //     return ranges::views::single(result);
  //   }
  //   return std::unexpected(Error::kInvalidOp);
  // }

  // Operand operator()(const StreamRef &ref) {
  //   if constexpr (OpResult<Op, bool, bool>::value) {
  //     google::protobuf::Value result;
  //     result.set_bool_value(Op()(_env.getEnv(ref)));
  //     return ranges::views::single(result);
  //   }
  //   return std::unexpected(Error::kInvalidOp);
  // }

  //

  // todo: implement ops for Stream, StreamRef, (binary) Word

  auto operator()(const IsValue auto &...v) const -> Operand {
    if (auto result = eval(v...)) {
      return std::visit([](auto value) -> Operand { return value; }, *result);
    } else {
      return [result](auto &) { return result; };
    }
  }
  auto operator()(const auto &...v) const -> Operand { return eval(v...); }

 private:
  auto eval(const IsValue auto &v) const -> Result<Value> {
    if (op == "+") return ValueOp<std::identity>()(v);
    if (op == "-") return ValueOp<std::negate<>>()(v);
    if (op == "!") return ValueOp<std::logical_not<>>()(v);
    return std::unexpected(Error::kInvalidOp);
  }
  auto eval(const ClosureValue &v) const -> ClosureValue {
    return [op = *this, v](const Closure &closure) {
      return v(closure).and_then(
          [&](Value v) { return std::visit([&](auto v) { return op.eval(v); }, v); });
    };
  }

  auto eval(const IsValue auto &lhs, const IsValue auto &rhs) const -> Result<Value> {
    if (op == "||") return ValueOp<std::logical_or<>>()(lhs, rhs);
    if (op == "&&") return ValueOp<std::logical_and<>>()(lhs, rhs);
    if (op == "==") return ValueOp<std::equal_to<>>()(lhs, rhs);
    if (op == "!=") return ValueOp<std::not_equal_to<>>()(lhs, rhs);
    if (op == "<") return ValueOp<std::less<>>()(lhs, rhs);
    if (op == "<=") return ValueOp<std::less_equal<>>()(lhs, rhs);
    if (op == ">") return ValueOp<std::greater<>>()(lhs, rhs);
    if (op == ">=") return ValueOp<std::greater_equal<>>()(lhs, rhs);
    if (op == "+") return ValueOp<std::plus<>>()(lhs, rhs);
    if (op == "-") return ValueOp<std::minus<>>()(lhs, rhs);
    if (op == "*") return ValueOp<std::multiplies<>>()(lhs, rhs);
    if (op == "/") return ValueOp<std::divides<>>()(lhs, rhs);
    if (op == "%") return ValueOp<std::modulus<>>()(lhs, rhs);
    if (op == "?") {
      return isTruthy(lhs) ? Result<Value>(rhs) : std::unexpected(Error::kCoalesceSkip);
    }
    if (op == "?:") return lhs;
    return std::unexpected(Error::kInvalidOp);
  }
  auto eval(const ClosureValue &lhs, const IsValue auto &rhs) const -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) {
      return lhs(closure)
          .and_then([&](Value lhs) {
            return std::visit([&](auto lhs) { return op.eval(lhs, rhs); }, lhs);
          })
          .or_else([&](Error err) -> Result<Value> {
            if (op.op == "?:" && err == Error::kCoalesceSkip) {
              return rhs;
            }
            return std::unexpected(err);
          });
    };
  }
  auto eval(const IsValue auto &lhs, const ClosureValue &rhs) const -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) {
      return rhs(closure).and_then(
          [&](Value rhs) { return std::visit([&](auto rhs) { return op.eval(lhs, rhs); }, rhs); });
    };
  }
  auto eval(const ClosureValue &lhs, const ClosureValue &rhs) const -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) {
      return rhs(closure).and_then([&](Value rhs) {
        return std::visit([&](auto rhs) { return op.eval(lhs, rhs)(closure); }, rhs);
      });
    };
  }
  auto eval(const auto &...) const -> ClosureValue {
    return [](auto &) { return std::unexpected(Error::kInvalidOp); };
  }

  std::string_view op;
};
