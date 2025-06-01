#pragma once

#include "stream-shell/to_stream.h"

inline Stream transform(ToStream to_stream, Stream input, auto fn) {
  return std::move(input) | ranges::views::for_each([to_stream = std::move(to_stream),
                                                     fn = std::move(fn)](auto result) {
           return std::move(result)
               .transform([&](auto value) { return std::visit(to_stream, fn(std::move(value))); })
               .or_else([&](Error err) -> Result<Stream> { return ranges::yield(result); })
               .value();
         });
}
