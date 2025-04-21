#pragma once

#include <range/v3/all.hpp>
#include "stream_parser.h"

auto lift(ranges::range auto rng)
    -> Result<ranges::any_view<std::decay_t<decltype(**ranges::begin(rng))>>> {
  using T = std::decay_t<decltype(**ranges::begin(rng))>;
  return ranges::fold_left(
      rng, ranges::views::empty<T>, [](Result<ranges::any_view<T>> &&rts, auto &&rt) {
        return rts.and_then([&](auto ts) {
          return std::forward<decltype(rt)>(rt).transform([&](T &&t) -> ranges::any_view<T> {
            return ranges::views::concat(ts, ranges::yield(std::forward<decltype(t)>(t)));
          });
        });
      });
}
