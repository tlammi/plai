#pragma once

#include <concepts>
#include <utility>

namespace plai {

template <std::invocable T>
class Defer {
 public:
    constexpr explicit Defer(T t) : m_t(std::move(t)) {}

    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;

    Defer(Defer&& other) noexcept : m_t(std::move(other.m_t)) {
        other.cancel();
    }

    Defer& operator=(Defer&& other) noexcept {
        auto tmp = Defer(std::move(other));
        std::swap(m_t, tmp.m_t);
        std::swap(m_cancelled, tmp.m_cancelled);
        return *this;
    }
    constexpr ~Defer() {
        if (!m_cancelled) m_t();
    }

    constexpr void cancel(bool val = true) noexcept { m_cancelled = val; }

 private:
    T m_t;
    bool m_cancelled = false;
};
}  // namespace plai
