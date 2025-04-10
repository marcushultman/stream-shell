#pragma once

#include <range/v3/all.hpp>
#include "stream_parser.h"
#include "stream_printer.h"
#include "tokenize.h"

// env.h
struct ProdEnv final : Env {
  StreamFactory getEnv(StreamRef) const override { return {}; }
  void setEnv(StreamRef, StreamFactory) override {}
};

inline void repl(Prompt prompt) {
  ProdEnv env;
  auto parser = makeStreamParser(env);

  for (const char *line; (line = prompt("stream-shell v0.1 🚀> "));) {
    printStream(parser->parse(tokenize(line) | ranges::to<std::vector>()),
                [&](auto s) { return prompt(s); });
  }
}
