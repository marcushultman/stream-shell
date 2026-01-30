#pragma once

#include <map>
#include <memory>
#include "stream_parser.h"

struct Scope {
  std::map<std::string, StreamFactory, std::less<>> env_overrides;
  std::map<std::string, std::shared_ptr<Value>, std::less<>> vars;

  auto add(std::string name) { return vars[std::move(name)] = std::make_shared<Value>(); }
};
