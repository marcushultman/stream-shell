#pragma once

#include "stream-shell/stream_parser.h"

struct TestEnv : Env {
  StreamFactory getEnv(StreamRef) const override { return {}; }
  void setEnv(StreamRef, StreamFactory) override {}
  bool sleepUntil(std::chrono::steady_clock::time_point) override { return true; }
  ssize_t read(int fd, google::protobuf::BytesValue &bytes) override { return -1; }
};
