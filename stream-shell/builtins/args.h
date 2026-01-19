#pragma once

#include <google/protobuf/struct.pb.h>
#include "stream-shell/config.h"
#include "stream-shell/stream_parser.h"

inline Stream args(const google::protobuf::Struct &config) {
  google::protobuf::Value value;
  auto args = toArgs(config);
  value.set_string_value(args | ranges::views::join(' ') | ranges::to<std::string>);
  return ranges::yield(value);
}
