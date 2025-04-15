#pragma once

#include <map>
#include <memory>
#include "stream_parser.h"

struct Word {
  Token value;
  auto operator<(const Word &rhs) const { return value < rhs.value; }

  friend bool operator<(const Word &lhs, const Token &rhs) { return lhs.value < rhs; }
  friend bool operator<(const Token &lhs, const Word &rhs) { return lhs < rhs.value; }
};

struct Closure {
  std::map<Word, StreamFactory, std::less<>> env_overrides;
  std::map<Word, std::shared_ptr<Value>, std::less<>> vars;

  auto add(const Word &name) { return vars[name] = std::make_shared<Value>(); }
};
