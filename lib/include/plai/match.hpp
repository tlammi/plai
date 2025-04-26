#pragma once

#include <utility>
#include <variant>

namespace plai {
namespace match_detail {

template <class... Callable>
struct visitor : Callable... {
    template <class... As>
    explicit visitor(As&&... as) : Callable(std::forward<As>(as))... {}
    using Callable::operator()...;
};

template <class... As>
visitor(As...) -> visitor<As...>;

}  // namespace match_detail

template <class Var, class... Callable>
constexpr decltype(auto) match(Var&& var, Callable&&... callables) {
    return std::visit(
        match_detail::visitor(std::forward<Callable>(callables)...),
        std::forward<Var>(var));
}
}  // namespace plai
