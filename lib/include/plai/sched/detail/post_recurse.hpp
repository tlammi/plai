#pragma once

#include <boost/asio.hpp>
#include <concepts>
#include <utility>

namespace plai::sched::detail {

struct no_arg_t {};

constexpr auto no_arg = no_arg_t{};

template <class T, class P>
decltype(auto) do_invoke(T&& t, P&& p) {
    if constexpr (std::same_as<std::remove_cvref_t<P>, no_arg_t>) {
        if constexpr (std::same_as<std::invoke_result_t<T>, void>) {
            std::forward<T>(t)();
            return no_arg;
        } else {
            return std::forward<T>(t)();
        }
    } else {
        if constexpr (std::same_as<std::invoke_result_t<T, P>, void>) {
            std::forward<T>(t)(std::forward<P>(p));
            return no_arg;
        } else {
            return std::forward<T>(t)(std::forward<P>(p));
        }
    }
}

template <class Exec, class P, class Fn, class... Fns>
void post_recurse(const Exec& exec, P&& p, Fn&& fn, Fns&&... fns) {
    if constexpr (!sizeof...(Fns)) {
        if constexpr (std::same_as<std::remove_cvref_t<P>, no_arg_t>) {
            boost::asio::post(exec, std::forward<Fn>(fn));
        } else {
            boost::asio::post(exec, [arg = std::forward<P>(p),
                                     fn = std::forward<Fn>(fn)]() mutable {
                std::move(fn)(std::move(arg));
            });
        }
    } else {
        boost::asio::post(
            exec, [exec, arg = std::forward<P>(p), fn = std::forward<Fn>(fn),
                   ... fns = std::forward<Fns>(fns)]() mutable {
                post_recurse(exec, do_invoke(std::move(fn), std::move(arg)),
                             std::move(fns)...);
            });
    }
}

}  // namespace plai::sched::detail
