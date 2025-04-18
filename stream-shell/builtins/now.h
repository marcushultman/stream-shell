#pragma once

#include <format>
#include <range/v3/all.hpp>
#include "stream-shell/stream_parser.h"

using namespace std::chrono_literals;

inline Stream now(Env &env) {
  auto start = std::chrono::system_clock::now();
  return ranges::views::iota(0) |
         ranges::views::transform([start](auto i) { return start + i * 1s; }) |
         ranges::views::take_while([&env](auto t) { return env.sleepUntil(t); }) |
         ranges::views::transform([](auto t) {
           google::protobuf::Value val;
           val.set_string_value(std::format("{:%FT%TZ}", t));
           return val;
         });
}
