#pragma once

#include <string_view>
#include <range/v3/all.hpp>

auto tokenize(ranges::any_view<const char, ranges::category::bidirectional>)
    -> ranges::any_view<ranges::any_view<const char, ranges::category::bidirectional>>;
