#pragma once

#include <concepts>
#include <utility>

namespace plai {

template <std::invocable T>
class Defer {
 public:
  constexpr explicit Defer(T t) : m_t(std::move(t)) {}
  constexpr ~Defer() {
    if (!m_cancelled) m_t();
  }

  constexpr void cancel(bool val = true) noexcept { m_cancelled = val; }

 private:
  T m_t;
  bool m_cancelled = false;
};
}  // namespace plai
