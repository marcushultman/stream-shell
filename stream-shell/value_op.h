#pragma once

#include "operand.h"
#include "stream_parser.h"

#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>

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

/**
 * Given a template functor, try performing the operation on 1 to 2 primitives.
 */
template <typename Op, typename Type = void>
struct ValueOp {
  using Result = ClosureValue::result_type;

  // Result operator()(const Stream &stream) {
  //   if constexpr (OpResult<Op, bool, bool>::value) {
  //     google::protobuf::Value result;
  //     result.set_bool_value(Op()(ranges::distance(Stream(stream))));
  //     return result;
  //   }
  //   return std::unexpected(Error::kInvalidOp);
  // }

  // Operand operator()(const StreamRef &ref) {
  //   if constexpr (OpResult<Op, bool, bool>::value) {
  //     google::protobuf::Value result;
  //     result.set_bool_value(Op()(_env.getEnv(ref)));
  //     return result;
  //   }
  //   return std::unexpected(Error::kInvalidOp);
  // }

  Result operator()(const google::protobuf::Value &val) {
    google::protobuf::Value result;
    if (val.has_number_value()) {
      if constexpr (std::is_invocable_r_v<bool, Op, bool> && std::is_same_v<Type, bool>) {
        result.set_bool_value(Op()(bool(val.number_value())));

      } else if constexpr (std::is_invocable_r_v<double, Op, double>) {
        result.set_number_value(Op()(val.number_value()));

      } else if constexpr (std::is_invocable_r_v<Stream, Op, int64_t>) {
        return Op()(int64_t(val.number_value()));

      } else {
        return std::unexpected(Error::kInvalidNumberOp);
      }

    } else if (val.has_bool_value()) {
      if constexpr (std::is_invocable_r_v<bool, Op, bool>) {
        result.set_bool_value(Op()(val.bool_value()));

      } else {
        return std::unexpected(Error::kInvalidBoolOp);
      }

    } else if (val.has_string_value()) {
      if constexpr (std::is_invocable_r_v<bool, Op, bool>) {
        result.set_bool_value(Op()(!val.string_value().empty()));

      } else {
        return std::unexpected(Error::kInvalidStringOp);
      }

    } else {
      return std::unexpected(Error::kInvalidOp);
    }
    return result;
  }

  Result operator()(const google::protobuf::Value &lhs, const google::protobuf::Value &rhs) {
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
        return std::unexpected(Error::kInvalidNumberOp);
      }

    } else if (lhs.has_bool_value() && rhs.has_bool_value()) {
      if constexpr (std::is_invocable_r_v<bool, Op, bool, bool>) {
        result.set_bool_value(Op()(lhs.bool_value(), rhs.bool_value()));

      } else {
        return std::unexpected(Error::kInvalidBoolOp);
      }

    } else if (lhs.has_string_value() && rhs.has_string_value()) {
      if constexpr (std::is_invocable_r_v<std::string,
                                          Op,
                                          const std::string &,
                                          const std::string &>) {
        result.set_string_value(Op()(lhs.string_value(), rhs.string_value()));

      } else if constexpr (std::is_invocable_r_v<bool,
                                                 Op,
                                                 const std::string &,
                                                 const std::string &>) {
        result.set_bool_value(Op()(lhs.string_value(), rhs.string_value()));

      } else {
        return std::unexpected(Error::kInvalidStringOp);
      }

    } else {
      return std::unexpected(Error::kInvalidOp);
    }
    return result;
  }

  Result operator()(const auto &...) { return std::unexpected(Error::kInvalidOp); }
};
