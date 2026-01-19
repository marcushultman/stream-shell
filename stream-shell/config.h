#pragma once

#include <span>
#include <string>
#include <vector>
#include <google/protobuf/struct.pb.h>
#include "operand.h"
#include "stream_parser.h"

Result<google::protobuf::Struct> toConfig(Env &, std::span<const Operand>);

std::vector<std::string> toArgs(const google::protobuf::Struct &config);
