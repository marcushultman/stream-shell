#pragma once

#include <csignal>
#include <google/protobuf/wrappers.pb.h>
#include <range/v3/all.hpp>
#include <unistd.h>
#include "env_impl.h"
#include "readline.h"
#include "stream_parser.h"
#include "stream_printer.h"
#include "tokenize.h"

static EnvImpl *s_env = nullptr;
static ReadlinePrompt *s_readline = nullptr;

inline void repl(ReadlinePrompt &readline) {
  EnvImpl env;
  auto parser = makeStreamParser(env);

  s_env = &env;
  s_readline = &readline;

  std::signal(SIGINT, [](int) {
    s_env->interrupt();
    s_readline->interrupt();
  });

  for (;;) {
    if (auto res = readline.prompt(ranges::yield("stream-shell v0.1 🚀> "), {'\t'})) {
      if (res->trigger == '\t') {
        std::cout << std::endl;
        parser->parseInteractive(tokenize(res->value));
      } else {
        printStream(parser->parse(tokenize(res->value)), readline);
      }
    } else {
      std::cout << std::endl;
    }
  }
}
