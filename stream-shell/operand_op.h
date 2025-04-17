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

template <typename T>
concept ValueOrStream = InVariant<ClosureValue::result_type::value_type, T>;

/**
 * Visits Operand variants transforming Values. For use in operators and builins
 */
template <typename Transform, typename ErrTransform = std::nullptr_t>
struct ValueTransform {
  ValueTransform(Transform &&transform, ErrTransform &&err_transform = {})
      : _transform(std::move(transform)), _err_transform(std::move(err_transform)) {}

  auto operator()(const ValueOrStream auto &...v) -> Operand {
    if (auto result = eval(v...)) {
      return std::visit([](auto value) -> Operand { return value; }, *result);
    } else {
      return [result](auto &) { return result; };
    }
  }
  auto operator()(const auto &...v) -> Operand { return eval(v...); }

 private:
  auto eval(const ValueOrStream auto &...) -> ClosureValue::result_type {
    return std::unexpected(Error::kInvalidOp);
  }

  auto eval(const IsValue auto &v) { return _transform(v); }
  auto eval(const ClosureValue &v) -> ClosureValue {
    return [op = *this, v](const Closure &closure) mutable {
      return v(closure).and_then(
          [&](auto v) { return std::visit([&](auto v) { return op.eval(v); }, v); });
    };
  }

  auto eval(const IsValue auto &lhs, const IsValue auto &rhs) { return _transform(lhs, rhs); }

  auto eval(const ClosureValue &lhs, const IsValue auto &rhs) -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) mutable {
      return lhs(closure)
          .and_then([&](auto lhs) {
            return std::visit([&](ValueOrStream auto lhs) { return op.eval(lhs, rhs); }, lhs);
          })
          .or_else([&](Error err) { return op._err_transform(err, rhs); });
    };
  }
  auto eval(const IsValue auto &lhs, const ClosureValue &rhs) -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) mutable {
      return rhs(closure).and_then(
          [&](auto rhs) { return std::visit([&](auto rhs) { return op.eval(lhs, rhs); }, rhs); });
    };
  }
  auto eval(const ClosureValue &lhs, const ClosureValue &rhs) -> ClosureValue {
    return [lhs, op = *this, rhs](const Closure &closure) mutable {
      return rhs(closure).and_then([&](auto rhs) {
        return std::visit([&](auto rhs) { return op.eval(lhs, rhs)(closure); }, rhs);
      });
    };
  }
  auto eval(const auto &...) -> ClosureValue {
    return [](auto &) { return std::unexpected(Error::kInvalidOp); };
  }

  Transform _transform;

 public:
  ErrTransform _err_transform;
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
