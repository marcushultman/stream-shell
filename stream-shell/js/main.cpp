#include <iostream>
#include <emscripten.h>
#include <emscripten/val.h>
#include <range/v3/all.hpp>
#include "stream-shell/stream_parser.h"
#include "stream-shell/stream_printer.h"
#include "stream-shell/tokenize.h"

const char *linenoise(std::string_view prompt) {
  static std::string buffer;
  (std::cout << prompt).flush();
  auto result = emscripten::val::global().call<emscripten::val>("getline").await();
  if (result.isNull()) {
    return nullptr;
  }
  buffer = result.as<std::string>();
  return buffer.c_str();
}

// env.h
struct ProdEnv final : Env {
  StreamFactory getEnv(StreamRef) const override { return {}; }
  void setEnv(StreamRef, StreamFactory) override {}
};

int main(int argc, char **argv) {
  ProdEnv env;
  auto parser = makeStreamParser(env);

  for (const char *line;;) {
    if (!(line = linenoise("stream-shell v0.1 🚀> "))) continue;
    printStream(parser->parse(tokenize(line) | ranges::to<std::vector>()),
                [] { return linenoise("Next [Enter]"); });
  }
  return 0;
}
