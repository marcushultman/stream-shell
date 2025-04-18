#pragma once

#include <format>
#include <range/v3/all.hpp>
#include "stream-shell/stream_parser.h"

using namespace std::chrono_literals;

inline Stream now(Env &env) {
  return ranges::views::iota(0) | ranges::views::transform([](auto i) { return i * 1s; }) |
         ranges::views::take_while([&env, start = std::chrono::steady_clock::now()](auto d) {
           return env.sleepUntil(start + d);
         }) |
         ranges::views::transform([start = std::chrono::system_clock::now()](auto d) {
           google::protobuf::Value val;
           val.set_string_value(std::format("{:%FT%TZ}", start + d));
           return val;
         });
}
