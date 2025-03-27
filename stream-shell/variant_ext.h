#pragma once

#include <type_traits>
#include <variant>

template <typename T, typename... Types>
struct variant_ext;

template <typename... T0, typename... T1>
struct variant_ext<std::variant<T0...>, T1...> : std::type_identity<std::variant<T0..., T1...>> {};

template <typename T, typename... Types>
using variant_ext_t = variant_ext<T, Types...>::type;

template <typename V, typename T>
struct variant_type;

template <typename... Types, typename T>
struct variant_type<std::variant<Types...>, T> : std::disjunction<std::is_same<T, Types>...> {};

template <typename V, typename T>
concept InVariant = variant_type<V, T>::value;
