#pragma once

#include <utility>

namespace plai {

template <class T>
struct Vec {
  T x;
  T y;

  constexpr explicit operator bool() const noexcept { return !zero(); }

  [[nodiscard]] bool zero() const noexcept { return !(x || y); }

  template <class T2>
  constexpr auto operator==(const Vec<T2>& other) const noexcept {
    return x == other.x && y == other.y;
  }
};

template <class T1, class T2>
constexpr Vec<T1> vec(T1&& l, T2&& r) {
  return {std::forward<T1>(l), std::forward<T2>(r)};
}

}  // namespace plai
