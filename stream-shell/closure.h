#pragma once

#include <map>
#include <memory>
#include <string_view>
#include "stream_parser.h"

struct Word {
  Word(Token value) : value{value} {}
  Token value;
  bool operator<(const Word &rhs) const { return value < rhs.value; }
};

struct Closure {
  std::map<Word, StreamFactory, std::less<>> env_overrides;
  std::map<Word, std::shared_ptr<Value>, std::less<>> vars;

  auto add(const Word &name) { return vars[name] = std::make_shared<Value>(); }
};
