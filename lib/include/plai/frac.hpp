#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <plai/util/parse.hpp>
#include <plai/util/str.hpp>
#include <utility>

namespace plai {

template <class T>
class Frac;

namespace concepts {
namespace detail {
template <class T>
struct is_frac : std::false_type {};

template <class T>
struct is_frac<Frac<T>> : std::true_type {};
}  // namespace detail

template <class T>
concept any_frac = detail::is_frac<T>::value;

template <class T, class U>
concept frac = std::same_as<T, Frac<U>>;

}  // namespace concepts
/**
 * \brief Fractional
 * */
template <class T>
class Frac {
 public:
    constexpr Frac(T num, T den) noexcept
        : m_num(std::move(num)), m_den(std::move(den)) {}

    template <std::floating_point F>
    explicit constexpr operator F() const noexcept {
        if (!m_den) return std::numeric_limits<F>::quiet_NaN();
        return static_cast<F>(m_num) / static_cast<F>(m_den);
    }

    constexpr Frac reciprocal() const noexcept { return {m_den, m_num}; }

    template <class S>
    constexpr decltype(auto) num(this S&& self) noexcept {
        return std::forward<S>(self).m_num;
    }

    template <class S>
    constexpr decltype(auto) den(this S&& self) noexcept {
        return std::forward<S>(self).m_den;
    }

    constexpr Frac& operator+=(const Frac& other) noexcept {
        assert(m_den == other.m_den);
        m_num += other.m_num;
        return *this;
    }

    constexpr Frac operator*(const T& scalar) const noexcept {
        T num = m_num * scalar;
        T den = m_den;
        if (!(num % den)) {
            num /= den;
            den = 1;
        }
        return {num, den};
    }

 private:
    T m_num{};
    T m_den{1};
};

template <class T>
constexpr Frac<T> operator*(const T& scalar, const Frac<T>& frac) noexcept {
    return frac * scalar;
}

namespace literals {
constexpr Frac<int64_t> operator""_frac(const char* str, size_t len) {
    auto v = std::string_view(str, len);
    auto [num_s, den_s] = split_left(v, "/");

    auto num = to_number<int64_t>(num_s);
    auto den = to_number<int64_t>(den_s);
    return {*num, *den};
}
}  // namespace literals

}  // namespace plai
