
#pragma once

#include "stream-shell/to_stream.h"

inline Stream prepend(ToStream &&to_stream, Stream input, auto args) {
  return ranges::views::concat(args | ranges::views::for_each(to_stream), std::move(input));
}
