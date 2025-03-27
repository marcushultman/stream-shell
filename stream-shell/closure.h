#pragma once

#include <map>
#include <memory>
#include "operand.h"
#include "stream_parser.h"

struct Closure {
  std::map<Word, std::shared_ptr<Value>, std::less<>> vars;
  auto add(const Word &name) { return vars[name] = std::make_shared<Value>(); }
};
