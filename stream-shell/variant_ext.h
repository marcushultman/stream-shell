#pragma once

#include <type_traits>
#include <variant>

template <typename T, typename... Types>
struct variant_ext;

template <typename... T0, typename... T1>
struct variant_ext<std::variant<T0...>, T1...> : std::type_identity<std::variant<T0..., T1...>> {};

template <typename T, typename... Types>
using variant_ext_t = variant_ext<T, Types...>::type;
