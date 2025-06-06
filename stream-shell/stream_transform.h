#pragma once

#include "stream-shell/stream_parser.h"

inline auto for_each(auto fn) {
  return ranges::views::for_each([fn = std::move(fn)](Result<Value> result) -> Stream {
    return result ? fn(std::move(*result)) : ranges::yield(std::move(result));
  });
}
