#pragma once

#include <condition_variable>
#include <mutex>
#include "stream_parser.h"

struct EnvImpl final : Env {
  EnvImpl();

  StreamFactory getEnv(StreamRef ref) const override;
  void setEnv(StreamRef ref, StreamFactory stream) override;
  bool blockUntilChange(StreamRef ref) override;

  bool sleepUntil(std::chrono::steady_clock::time_point t) override;
  ssize_t read(int fd, google::protobuf::BytesValue &bytes) override;

  void interrupt();

 private:
  void load(std::string path);

  std::vector<std::string> _config;
  std::unique_ptr<StreamParser> _parser;

  struct EnvEntry {
    StreamFactory stream;
    int version = 0;
  };
  mutable std::map<StreamRef, EnvEntry, std::less<>> _cache;
  std::condition_variable _cv;
  std::mutex _mutex;
  bool _stop = false;
};
