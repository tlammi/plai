#pragma once

#include <cstddef>
#include <plai/concepts.hpp>
#include <plai/exceptions.hpp>
#include <plai/st/tag.hpp>
#include <plai/thirdparty/magic_enum.hpp>

namespace plai::st::detail {

template <concepts::enum_type E>
constexpr auto state_count = magic_enum::enum_count<E>();

template <concepts::enum_type E>
constexpr auto init_state = magic_enum::enum_value<E>(0);

template <concepts::enum_type E>
constexpr auto done_state = magic_enum::enum_value<E>(state_count<E> - 1);

template <size_t Idx, class T, concepts::enum_type E>
constexpr decltype(auto) tag_step(T& t, E e) {
    constexpr auto enum_value = magic_enum::enum_value<E>(Idx);
    if (e == enum_value) { return t.step(tag<enum_value>); }
    if constexpr (Idx < state_count<E> - 2)
        return tag_step<Idx + 1>(t, e);
    else
        throw ValueError("Invalid state");
}
}  // namespace plai::st::detail
