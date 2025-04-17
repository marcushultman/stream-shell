#pragma once

#include <format>
#include <thread>
#include <range/v3/all.hpp>
#include "stream-shell/stream_parser.h"

using namespace std::chrono_literals;

inline Stream now() {
  auto start = std::chrono::system_clock::now();
  return ranges::views::iota(1) | ranges::views::transform([start](auto i) {
           auto next = start + i * 1s;
           std::this_thread::sleep_until(next);
           google::protobuf::Value val;
           val.set_string_value(std::format("{:%FT%TZ}", next));
           return val;
         });
}
