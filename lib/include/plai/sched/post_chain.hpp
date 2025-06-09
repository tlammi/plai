#pragma once

#include <plai/sched/detail/post_recurse.hpp>
namespace plai::sched {
namespace detail {
template <class T>
concept executor_provider = requires(T t) {
    { t.get_executor() };
};
}  // namespace detail

template <class Exec, class... Fns>
void post_chain(Exec&& exec, Fns&&... fns) {
    if constexpr (detail::executor_provider<Exec>) {
        detail::post_recurse(exec.get_executor(), detail::no_arg,
                             std::forward<Fns>(fns)...);
    } else {
        detail::post_recurse(std::forward<Exec>(exec), detail::no_arg,
                             std::forward<Fns>(fns)...);
    }
}

}  // namespace plai::sched
