#pragma once

#include <plai/st/detail.hpp>
#include <plai/st/tag.hpp>
#include <plai/thirdparty/magic_enum.hpp>

namespace plai::st {

template <class T>
class StateMachine {
 public:
    using state_type = T::state_type;
    static constexpr auto state_count = magic_enum::enum_count<state_type>();
    static_assert(state_count >= 2, "Must have at least two states");
    static constexpr auto init_state = magic_enum::enum_value<state_type>(0);
    static constexpr auto done_state =
        magic_enum::enum_value<state_type>(state_count - 1);

    constexpr StateMachine() = default;
    constexpr explicit StateMachine(const T& t) : m_impl(t) {}
    constexpr explicit StateMachine(T&& t) : m_impl(std::move(t)) {}

    template <class... Ts>
    constexpr explicit StateMachine(std::in_place_t /*unused*/, Ts&&... ts)
        : m_impl(std::forward<Ts>(ts)...) {}

    void operator()() { m_st = detail::tag_step<0>(m_impl, m_st); }

    constexpr void reset() noexcept { m_st = init_state; }

    constexpr state_type state() const noexcept { return m_st; }
    constexpr bool done() const noexcept { return m_st == done_state; }

 private:
    T m_impl{};
    state_type m_st{magic_enum::enum_value<state_type>(0)};
};

}  // namespace plai::st
