#pragma once

#include <concepts>
#include <type_traits>

namespace plai::concepts {
namespace detail {

template <class T, class Proto>
struct prototype_impl;

template <class T, class R, class... Ts>
struct prototype_impl<T, R(Ts...)> {
    static constexpr bool value_invocable = std::invocable<T, Ts...>;
    static constexpr bool value_ret =
        std::same_as<std::invoke_result_t<T, Ts...>, R>;

    static constexpr bool value = value_invocable && value_ret;
};
}  // namespace detail

template <class T>
concept enum_type = std::is_enum_v<T>;

template <class T, class Proto>
concept prototype = detail::prototype_impl<T, Proto>::value;
}  // namespace plai::concepts
