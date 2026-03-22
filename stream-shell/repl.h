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
    auto future = readline.prompt("stream-shell v0.1 🚀> ");

    // todo: refresh prompt

    if (auto line = future.get()) {
      printStream(parser->parse(tokenize(*line)), readline);
    } else {
      break;
    }
  }
}
