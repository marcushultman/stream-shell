#pragma once

#include <condition_variable>
#include <csignal>
#include <fstream>
#include <google/protobuf/wrappers.pb.h>
#include <range/v3/all.hpp>
#include <unistd.h>
#include "stream_parser.h"
#include "stream_printer.h"
#include "tokenize.h"

using namespace std::string_view_literals;

// env.h
struct ProdEnv final : Env {
  ProdEnv() {
    if (auto *xdg_data_home_dir = getenv("XDG_DATA_HOME")) {
      load(xdg_data_home_dir);
    } else if (auto *home_dir = getenv("HOME")) {
      load(std::string(home_dir) + "/.config");
    }
    setEnv({"STSH_VERSION"sv}, [](auto) {
      auto value = google::protobuf::Value();
      value.set_string_value(STSH_VERSION);
      return ranges::yield(value);
    });
  }

  StreamFactory getEnv(StreamRef ref) const override {
    if (auto it = _cache.find(ref); it != _cache.end()) {
      return it->second;
    } else if (auto str = std::getenv((ref.name | ranges::to<std::string>).c_str())) {
      return _cache[ref] = [sv = std::string_view(str)](auto) {
        return sv | ranges::views::split(':') | ranges::views::transform([](auto chunk) {
                 google::protobuf::Value value;
                 value.set_string_value(chunk | ranges::to<std::string>);
                 return value;
               });
      };
    }
    return {};
  }
  void setEnv(StreamRef ref, StreamFactory stream) override {
    // todo: setenv?
    _cache[ref] = std::move(stream);
  }
  bool sleepUntil(std::chrono::steady_clock::time_point t) override {
    std::unique_lock lock(_mutex);
    _stop = false;
    for (; !_stop && _cv.wait_until(lock, t) != std::cv_status::timeout;);
    return !_stop;
  }
  ssize_t read(int fd, google::protobuf::BytesValue &bytes) override {
    std::unique_lock lock(_mutex);
    _stop = false;
    lock.unlock();
    bytes.mutable_value()->resize(4096);
    auto ret = ::read(fd, bytes.mutable_value()->data(), bytes.mutable_value()->size());
    if (ret > 0) {
      bytes.mutable_value()->resize(ret);
    }
    lock.lock();
    return !_stop ? ret : -1;
  }

  void interrupt() {
    std::unique_lock lock(_mutex);
    _stop = true;
    _cv.notify_all();
  }

 private:
  void load(std::string path) {
    std::ifstream config(path + "/stream-shell/config.st", std::ios::in);
    if (!config.is_open()) {
      return;
    }
    _config = (std::stringstream() << config.rdbuf()).view() | ranges::views::split('\n') |
              ranges::views::filter(ranges::distance) | ranges::to<std::vector<std::string>>;
    for (auto &line : _config) {
      (void)_parser->parse(tokenize(line));
    }
  }

  std::vector<std::string> _config;
  std::unique_ptr<StreamParser> _parser = makeStreamParser(*this);
  mutable std::map<StreamRef, StreamFactory, std::less<>> _cache;
  std::condition_variable _cv;
  std::mutex _mutex;
  bool _stop = false;
};

static ProdEnv *s_env = nullptr;

inline void repl(Prompt prompt) {
  ProdEnv env;
  s_env = &env;
  auto parser = makeStreamParser(env);

  for (const char *line; (line = prompt("stream-shell v0.1 🚀> "));) {
    std::signal(SIGINT, [](int) { s_env->interrupt(); });
    printStream(parser->parse(tokenize(std::string_view(line))), [&](auto s) { return prompt(s); });
    std::signal(SIGINT, nullptr);
  }
}
