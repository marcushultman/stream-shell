#pragma once

#include "stream_parser.h"

#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>

/**
 * Given a template functor, try performing the operation on 1 to 2 primitives.
 */
template <typename Op, typename Type = void>
struct ValueOp {
  Stream operator()(const google::protobuf::Value &val) {
    google::protobuf::Value result;
    if (val.has_number_value()) {
      if constexpr (std::is_invocable_r_v<bool, Op, bool> && std::is_same_v<Type, bool>) {
        result.set_bool_value(Op()(bool(val.number_value())));

      } else if constexpr (std::is_invocable_r_v<double, Op, double>) {
        result.set_number_value(Op()(val.number_value()));

      } else if constexpr (std::is_invocable_r_v<Stream, Op, int64_t>) {
        return Op()(int64_t(val.number_value()));

      } else {
        return ranges::yield(std::unexpected(Error::kInvalidNumberOp));
      }

    } else if (val.has_bool_value()) {
      if constexpr (std::is_invocable_r_v<bool, Op, bool>) {
        result.set_bool_value(Op()(val.bool_value()));

      } else {
        return ranges::yield(std::unexpected(Error::kInvalidBoolOp));
      }

    } else if (val.has_string_value()) {
      if constexpr (std::is_invocable_r_v<bool, Op, bool>) {
        result.set_bool_value(Op()(!val.string_value().empty()));

      } else {
        return ranges::yield(std::unexpected(Error::kInvalidStringOp));
      }

    } else {
      return ranges::yield(std::unexpected(Error::kInvalidOp));
    }
    return ranges::yield(result);
  }

  Stream operator()(const google::protobuf::Value &lhs, const google::protobuf::Value &rhs) {
    google::protobuf::Value result;
    if (lhs.has_number_value() && rhs.has_number_value()) {
      if constexpr (std::is_invocable_r_v<bool, Op, bool, bool> && std::is_same_v<Type, bool>) {
        result.set_bool_value(Op()(bool(lhs.number_value()), bool(rhs.number_value())));

      } else if constexpr (std::is_invocable_r_v<double, Op, double, double>) {
        result.set_number_value(Op()(lhs.number_value(), rhs.number_value()));

      } else if constexpr (std::is_invocable_r_v<int64_t, Op, int64_t, int64_t>) {
        result.set_number_value(Op()(int64_t(lhs.number_value()), int64_t(rhs.number_value())));

      } else if constexpr (std::is_invocable_r_v<Stream, Op, int64_t, int64_t>) {
        return Op()(int64_t(lhs.number_value()), int64_t(rhs.number_value()));

      } else {
        return ranges::yield(std::unexpected(Error::kInvalidNumberOp));
      }

    } else if (lhs.has_bool_value() && rhs.has_bool_value()) {
      if constexpr (std::is_invocable_r_v<bool, Op, bool, bool>) {
        result.set_bool_value(Op()(lhs.bool_value(), rhs.bool_value()));

      } else {
        return ranges::yield(std::unexpected(Error::kInvalidBoolOp));
      }

    } else if (lhs.has_string_value() && rhs.has_string_value()) {
      if constexpr (std::is_invocable_r_v<std::string, Op, std::string, std::string>) {
        result.set_string_value(Op()(lhs.string_value(), rhs.string_value()));

      } else if constexpr (std::is_invocable_r_v<bool, Op, std::string, std::string>) {
        result.set_bool_value(Op()(lhs.string_value(), rhs.string_value()));

      } else {
        return ranges::yield(std::unexpected(Error::kInvalidStringOp));
      }

    } else {
      return ranges::yield(std::unexpected(Error::kInvalidOp));
    }
    return ranges::yield(result);
  }

  Stream operator()(const auto &...) { return ranges::yield(std::unexpected(Error::kInvalidOp)); }
};
