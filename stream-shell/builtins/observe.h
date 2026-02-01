#pragma once

#include <google/protobuf/struct.pb.h>
#include "stream-shell/stream_parser.h"

inline Stream observe(Env &env, const google::protobuf::Struct &config) {
  if (auto it = config.fields().find("@"); it == config.fields().end()) {
    return ranges::yield(std::unexpected(Error::kInvalidOp));

  } else if (it->second.list_value().values().size() != 1 ||
             !it->second.list_value().values().at(0).has_string_value()) {
    return ranges::yield(std::unexpected(Error::kInvalidOp));

  } else {
    auto ref = StreamRef{it->second.list_value().values().at(0).string_value()};
    if (auto factory = env.getEnv(ref)) {
      return ranges::views::iota(0) | ranges::views::for_each([&env, ref, factory](auto) {
               return ranges::views::concat(
                   factory({}), ranges::yield(0) | ranges::views::for_each([&](auto) -> Stream {
                                  if (env.blockUntilChange(ref)) {
                                    return {};
                                  }
                                  return ranges::yield(std::unexpected(Error::kAborted));
                                }));
             });
    }
  }
  return ranges::yield(std::unexpected(Error::kInvalidOp));
}
