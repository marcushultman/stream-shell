#pragma once

#include <fstream>
#include <range/v3/all.hpp>
#include <unistd.h>
#include "stream_parser.h"
#include "stream_printer.h"
#include "tokenize.h"

extern char **environ;

using namespace std::string_view_literals;

// env.h
struct ProdEnv final : Env {
  ProdEnv() {
    if (auto *xdg_data_home_dir = getenv("XDG_DATA_HOME")) {
      load(xdg_data_home_dir);
    } else if (auto *home_dir = getenv("HOME")) {
      load(std::string(home_dir) + "/.config");
    }
  }

  StreamFactory getEnv(StreamRef ref) const override {
    if (auto it = _cache.find(ref); it != _cache.end()) {
      return it->second;
    } else if (auto var = std::getenv((ref.name | ranges::to<std::string>).c_str())) {
      auto [stream, _] = _parser->parse(tokenize(std::string_view(var)));
      return _cache[ref] = [stream] { return stream; };
    }
    return {};
  }
  void setEnv(StreamRef ref, StreamFactory stream) override {
    // todo: setenv?
    _cache[ref] = std::move(stream);
  }

 private:
  void load(std::string path) {
    std::ifstream config(path + "/stream-shell/config.st", std::ios::in);
    if (!config.is_open()) {
      return;
    }
    std::stringstream buffer;
    buffer << config.rdbuf();
    _config = buffer.str();
    (void)_parser->parse(tokenize(_config));
  }

  std::string _config;
  std::unique_ptr<StreamParser> _parser = makeStreamParser(*this);
  mutable std::map<StreamRef, StreamFactory, std::less<>> _cache;
};

inline void repl(Prompt prompt) {
  ProdEnv env;
  auto parser = makeStreamParser(env);

  for (const char *line; (line = prompt("stream-shell v0.1 🚀> "));) {
    printStream(parser->parse(tokenize(std::string_view(line))), [&](auto s) { return prompt(s); });
  }
}
