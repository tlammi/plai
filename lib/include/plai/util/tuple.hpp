#pragma once
#include <cstddef>
#include <stdexcept>
#include <tuple>

namespace plai {
namespace tuple_detail {

template <size_t Idx, class Res, class Tuple>
Res& get_nth_impl(Tuple& t, size_t idx) {
    if constexpr (Idx < std::tuple_size_v<Tuple>) {
        if (idx == Idx) return std::get<Idx>(t);
        return get_nth_impl<Idx + 1, Res>(t, idx);
    }
    throw std::runtime_error("Tuple index out of bounds");
}
}  // namespace tuple_detail

template <class Res, class Tuple>
Res& get_nth(Tuple& t, size_t idx) {
    return tuple_detail::get_nth_impl<0, Res>(t, idx);
}
}  // namespace plai
