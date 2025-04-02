#pragma once

#include "stream_parser.h"

#include <expected>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <range/v3/all.hpp>

template <typename Op, typename T, typename R, typename = void>
struct OpResult : std::false_type {};

template <typename Op, typename T, typename R>
struct OpResult<Op, T, R, std::enable_if_t<std::is_same_v<std::invoke_result_t<Op, T, T>, R>>>
    : std::true_type {};

template <typename Op, typename T, typename R>
struct OpResult<Op, T, R, std::enable_if_t<std::is_same_v<std::invoke_result_t<Op, T>, R>>>
    : std::true_type {};

template <typename Op>
struct ValueOp {
  using Result = std::expected<Value, std::string>;

  ValueOp(const Env &env) : _env{env} {}

  Result operator()(const google::protobuf::Value &val) {
    google::protobuf::Value result;
    if (val.has_number_value()) {
      if constexpr (OpResult<Op, double, double>::value) {
        result.set_number_value(Op()(val.number_value()));

      } else if constexpr (OpResult<Op, double, bool>::value) {
        result.set_bool_value(Op()(val.number_value()));

      } else {
        return std::unexpected("can't apply op to numbers");
      }

    } else if (val.has_bool_value()) {
      if constexpr (OpResult<Op, bool, bool>::value) {
        result.set_bool_value(Op()(val.bool_value()));

      } else {
        return std::unexpected("can't apply op to bool");
      }

    } else if (val.has_string_value()) {
      if constexpr (OpResult<Op, bool, bool>::value) {
        result.set_bool_value(Op()(!val.string_value().empty()));

      } else {
        return std::unexpected("can't apply op to bool");
      }

    } else {
      return std::unexpected("can't apply op");
    }
    return result;
  }

  Result operator()(const Stream &stream) {
    if constexpr (OpResult<Op, bool, bool>::value) {
      google::protobuf::Value result;
      result.set_bool_value(Op()(ranges::distance(Stream(stream))));
      return result;
    }
    return std::unexpected("Operator expected Value");
  }
  Result operator()(const StreamRef &ref) {
    if constexpr (OpResult<Op, bool, bool>::value) {
      google::protobuf::Value result;
      result.set_bool_value(Op()(_env.getEnv(ref)));
      return result;
    }
    return std::unexpected("Operator expected Value");
  }

  Result operator()(const google::protobuf::Value &lhs, const google::protobuf::Value &rhs) {
    google::protobuf::Value result;
    if (lhs.has_number_value() && rhs.has_number_value()) {
      if constexpr (OpResult<Op, double, double>::value) {
        result.set_number_value(Op()(lhs.number_value(), rhs.number_value()));

      } else if constexpr (OpResult<Op, double, bool>::value) {
        result.set_bool_value(Op()(lhs.number_value(), rhs.number_value()));

      } else if constexpr (OpResult<Op, int64_t, int64_t>::value) {
        result.set_number_value(Op()(int64_t(lhs.number_value()), int64_t(rhs.number_value())));

      } else {
        return std::unexpected("can't apply op to numbers");
      }

    } else if (lhs.has_bool_value() && rhs.has_bool_value()) {
      if constexpr (OpResult<Op, bool, bool>::value) {
        result.set_bool_value(Op()(lhs.bool_value(), rhs.bool_value()));

      } else {
        return std::unexpected("can't apply op to bools");
      }

    } else if (lhs.has_string_value() && rhs.has_string_value()) {
      if constexpr (OpResult<Op, const std::string &, std::string>::value) {
        result.set_string_value(Op()(lhs.string_value(), rhs.string_value()));

      } else if constexpr (OpResult<Op, const std::string &, bool>::value) {
        result.set_bool_value(Op()(lhs.string_value(), rhs.string_value()));

      } else {
        return std::unexpected("can't apply op to strings");
      }

    } else {
      return std::unexpected("can't apply op");
    }
    return result;
  }
  Result operator()(const auto &...) { return std::unexpected("Operator expected Value"); }

  const Env &_env;
};
