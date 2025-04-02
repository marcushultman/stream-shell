#include <range/v3/all.hpp>
#include "linenoise.h"
#include "stream_parser.h"
#include "stream_printer.h"
#include "tokenize.h"

// env.h
struct ProdEnv final : Env {
  StreamFactory getEnv(StreamRef) const override { return {}; }
  void setEnv(StreamRef, StreamFactory) override {}
};

int main(int argc, char **argv) {
  ProdEnv env;
  auto parser = makeStreamParser(env);

  for (const char *line; (line = linenoise("stream-shell v0.1 🚀> "));) {
    printStream(parser->parse(tokenize(line) | ranges::to<std::vector>()));
  }
  return 0;
}
