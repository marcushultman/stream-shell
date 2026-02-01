#pragma once

#include <csignal>
#include <google/protobuf/wrappers.pb.h>
#include <range/v3/all.hpp>
#include <unistd.h>
#include "env_impl.h"
#include "stream_parser.h"
#include "stream_printer.h"
#include "tokenize.h"

static EnvImpl *s_env = nullptr;

inline void repl(Prompt prompt) {
  EnvImpl env;
  s_env = &env;
  auto parser = makeStreamParser(env);

  for (const char *line; (line = prompt("stream-shell v0.1 🚀> "));) {
    std::signal(SIGINT, [](int) { s_env->interrupt(); });
    printStream(parser->parse(tokenize(std::string_view(line))), [&](auto s) { return prompt(s); });
    std::signal(SIGINT, nullptr);
  }
}
