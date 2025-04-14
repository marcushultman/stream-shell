#pragma once

#include <fstream>
#include <range/v3/all.hpp>
#include <unistd.h>
#include "stream_parser.h"
#include "stream_printer.h"
#include "tokenize.h"

extern char **environ;

using namespace std::string_view_literals;

static auto kEnvVars =
    ranges::views::iota(0) | ranges::views::transform([](auto i) { return environ[i]; }) |
    ranges::views::take_while(std::identity()) |
    ranges::views::transform([](auto var) { return std::string_view(var); }) |
    ranges::views::transform([](auto var) { return ranges::views::concat("$"sv, var); }) |
    ranges::views::transform([](auto var) { return tokenize(var); }) | ranges::to<std::vector>;

// env.h
struct ProdEnv final : Env {
  ProdEnv() {
    auto env_parser = makeStreamParser(*this);

    for (auto &expr : kEnvVars) {
      env_parser->parse(expr);
    }

    if (auto *xdg_data_home_dir = getenv("XDG_DATA_HOME")) {
      load(*env_parser, xdg_data_home_dir);
    } else if (auto *home_dir = getenv("HOME")) {
      load(*env_parser, std::string(home_dir) + "/.config");
    }
  }

  StreamFactory getEnv(StreamRef ref) const override {
    if (auto it = _env_vars.find(ref); it != _env_vars.end()) {
      return it->second;
    }
    return {};
  }
  void setEnv(StreamRef ref, StreamFactory stream) override {
    _env_vars.emplace(ref, std::move(stream));
  }

 private:
  void load(StreamParser &parser, std::string path) {
    std::ifstream config(path + "/stream-shell/config.st", std::ios::in);
    if (!config.is_open()) {
      return;
    }
    std::stringstream buffer;
    buffer << config.rdbuf();
    _config = buffer.str();
    (void)parser.parse(tokenize(_config));
  }

  std::string _config;
  std::map<StreamRef, StreamFactory, std::less<>> _env_vars;
};

inline void repl(Prompt prompt) {
  ProdEnv env;
  auto parser = makeStreamParser(env);

  for (const char *line; (line = prompt("stream-shell v0.1 🚀> "));) {
    printStream(parser->parse(tokenize(std::string_view(line))), [&](auto s) { return prompt(s); });
  }
}
