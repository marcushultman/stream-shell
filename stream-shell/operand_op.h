#pragma once

#include <expected>
#include <functional>
#include <type_traits>
#include "closure.h"
#include "operand.h"
#include "stream_parser.h"
#include "value_op.h"

template <typename T>
concept NonWord = !std::is_same_v<std::decay_t<T>, Word>;

struct OperandOp {
  template <typename T>
  using Result = std::expected<T, std::string>;

  OperandOp(const Env &env, const Closure &closure, std::string_view op)
      : env{env}, closure{closure}, op{op} {}

  static auto unary(const Env &env, auto op, const auto &val) -> Result<Value> {
    if (op == "-") return ValueOp<std::negate<>>(env)(val);
    if (op == "!") return ValueOp<std::logical_not<>>(env)(val);
    assert(0);
  }

  static auto binary(const Env &env, auto op, const auto &lhs, const auto &rhs) -> Result<Value> {
    if (op == "||") return ValueOp<std::logical_or<>>(env)(lhs, rhs);
    if (op == "&&") return ValueOp<std::logical_and<>>(env)(lhs, rhs);
    if (op == "==") return ValueOp<std::equal_to<>>(env)(lhs, rhs);
    if (op == "!=") return ValueOp<std::not_equal_to<>>(env)(lhs, rhs);
    if (op == "<") return ValueOp<std::less<>>(env)(lhs, rhs);
    if (op == "<=") return ValueOp<std::less_equal<>>(env)(lhs, rhs);
    if (op == ">") return ValueOp<std::greater<>>(env)(lhs, rhs);
    if (op == ">=") return ValueOp<std::greater_equal<>>(env)(lhs, rhs);
    if (op == "+") return ValueOp<std::plus<>>(env)(lhs, rhs);
    if (op == "-") return ValueOp<std::minus<>>(env)(lhs, rhs);
    if (op == "*") return ValueOp<std::multiplies<>>(env)(lhs, rhs);
    if (op == "/") return ValueOp<std::divides<>>(env)(lhs, rhs);
    if (op == "%") return ValueOp<std::modulus<>>(env)(lhs, rhs);
    assert(0);
  }

  auto varOp(const Word &var, auto &&transform) -> Result<Operand> {
    if (auto it = closure.vars.find(var); it != closure.vars.end()) {
      return ranges::views::single(ranges::ref(*it->second)) |
             ranges::views::transform([transform = std::move(transform)](const Value &value) {
               return std::visit(
                   [&](auto &value) -> Value {
                     if (auto result = transform(value)) {
                       return *result;
                     }
                     google::protobuf::Value null;
                     null.set_null_value(google::protobuf::NULL_VALUE);
                     return null;
                   },
                   value);
             });
    }
    return std::unexpected(std::format("'{}' is not a variable", var.value));
  }

  // todo: implement ops for Stream, StreamRef, Word

  auto operator()(const Word &v) -> Result<Operand> {
    return varOp(v, [&env = env, op = op](auto &val) { return unary(env, op, val); });
  }
  auto operator()(const NonWord auto &lhs, const Word &rhs) -> Result<Operand> {
    return varOp(rhs, [&env = env, op = op, lhs](auto &rhs) { return binary(env, op, lhs, rhs); });
  }
  auto operator()(const Word &lhs, const NonWord auto &rhs) -> Result<Operand> {
    return varOp(lhs, [&env = env, op = op, rhs](auto &lhs) { return binary(env, op, lhs, rhs); });
  }
  auto operator()(const auto &v) -> Result<Operand> {
    return unary(env, op, v).transform([](auto &&v) { return toOperand(std::move(v)); });
  }
  auto operator()(const auto &...v) -> Result<Operand> {
    return binary(env, op, v...).transform([](auto &&v) { return toOperand(std::move(v)); });
  }

  const Env &env;
  const Closure &closure;
  std::string_view op;
};
