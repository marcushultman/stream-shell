#pragma once

#include <map>
#include <memory>
#include <string_view>
#include "stream_parser.h"

struct Word {
  Word(const std::string &value) : value{value} {}
  Word(std::string_view value) : value{value} {}
  std::string_view value;
  std::strong_ordering operator<=>(const Word &) const = default;
};

struct Closure {
  std::map<Word, StreamFactory, std::less<>> env_overrides;
  std::map<Word, std::shared_ptr<Value>, std::less<>> vars;

  auto add(const Word &name) { return vars[name] = std::make_shared<Value>(); }
};
