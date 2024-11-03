#pragma once

#include <variant>

namespace plai {
namespace match_detail {
template <class... Ts>
struct Visitor final : Ts... {
    using Ts::operator()...;
};

}  // namespace match_detail

template <class T, class... Ts>
decltype(auto) match(T&& t, Ts&&... ts) {
    return std::visit(match_detail::Visitor<Ts...>{std::forward<Ts>(ts)...},
                      std::forward<T>(t));
}

}  // namespace plai
