#pragma once

#include <string_view>
#include <range/v3/all.hpp>

auto tokenize(std::string_view) -> ranges::any_view<std::string_view>;
