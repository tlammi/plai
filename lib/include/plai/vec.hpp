#pragma once

#include <algorithm>
#include <concepts>
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

    /**
     * \brief Scale the vector so it is bound by the argument
     *
     * This scales the vector up/down without distortions so one of its
     * dimension matches the bounds corresponding dimensions and the other one
     * is smaller.
     * */
    template <class U>
    void scale_to(const Vec<U>& bound) noexcept {
        const double x_aspect = double(bound.x) / x;
        const double y_aspect = double(bound.y) / y;
        *this *= std::min(x_aspect, y_aspect);
    }

    template <class U>
    constexpr Vec& operator*=(const U& val) noexcept {
        x *= val;
        y *= val;
        return *this;
    }
};

template <class T1, class T2>
constexpr Vec<T1> vec(T1&& l, T2&& r) {
    return {std::forward<T1>(l), std::forward<T2>(r)};
}

}  // namespace plai
