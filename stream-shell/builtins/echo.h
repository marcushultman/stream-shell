#pragma once

#include <google/protobuf/struct.pb.h>
#include "stream-shell/stream_parser.h"

inline Stream echo(const google::protobuf::Struct &config) {
  google::protobuf::Value value;
  *value.mutable_struct_value() = config;
  return ranges::yield(value);
}
