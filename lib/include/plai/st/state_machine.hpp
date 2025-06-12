#pragma once

#include <plai/logs/logs.hpp>
#include <plai/st/detail.hpp>
#include <plai/st/tag.hpp>
#include <plai/thirdparty/magic_enum.hpp>
#include <print>

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

    template <class... Ps>
    constexpr void operator()(Ps&&... ps) {
        assert(!done());
        auto prev_st = m_st;
        m_st = detail::tag_step<0>(m_impl, m_st, std::forward<Ps>(ps)...);
        if (m_st != prev_st)
            PLAI_DEBUG("Reached state {}::{}",
                       magic_enum::enum_type_name<state_type>(),
                       magic_enum::enum_name(m_st));
    }

    constexpr void reset() noexcept {
        m_st = init_state;
        m_impl.reset();
    }

    template <class... Ts>
    void emplace(Ts&&... ts) {
        m_impl = T(std::forward<Ts>(ts)...);
    }

    constexpr state_type state() const noexcept { return m_st; }
    constexpr bool initial() const noexcept { return m_st == init_state; }
    constexpr bool done() const noexcept { return m_st == done_state; }

    T& operator*() noexcept { return m_impl; }
    const T& operator*() const noexcept { return m_impl; }

    T* operator->() noexcept { return &m_impl; }
    const T* operator->() const noexcept { return &m_impl; }

 private:
    T m_impl{};
    state_type m_st{magic_enum::enum_value<state_type>(0)};
};

}  // namespace plai::st
