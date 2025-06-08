#pragma once

#include <algorithm>
#include <concepts>

namespace plai {
namespace algo_detail {
template <class F, class S, class... Rest>
F max_of_impl(const F& first, const S& second, const Rest&... rest) {
    if constexpr (sizeof...(Rest) == 0) {
        return std::max(first, second);
    } else {
        return max_of_impl(std::max(first, second), rest...);
    }
}

}  // namespace algo_detail
template <class F, std::convertible_to<F>... Ts>
F max_of(const F& f, const Ts&... ts) {
    if constexpr (sizeof...(Ts) == 0) {
        return f;
    } else {
        return algo_detail::max_of_impl(f, ts...);
    }
}
}  // namespace plai
