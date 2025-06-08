#pragma once

#include <concepts>
#include <cstddef>
#include <tuple>

namespace plai::sched {

template <class... Steps>
class Task {
    using Tuple = std::tuple<Steps...>;

 public:
    explicit Task(std::tuple<Steps...> steps) noexcept
        : m_steps(std::move(steps)) {}

    constexpr size_t step_count() const noexcept {
        return std::tuple_size_v<Tuple>;
    }

 private:
    Tuple m_steps{};
};

struct TaskFinish {};
constexpr auto task_finish() { return TaskFinish{}; }

template <class Fn, class... Steps>
struct TaskBuilder {
    Fn last;
    std::tuple<Steps...> preceding;

    template <class Fn2>
    auto operator|(Fn2&& fn) && {
        return TaskBuilder<Fn2, Steps..., Fn>{
            .last = std::forward<Fn2>(fn),
            .preceding = std::tuple_cat(std::move(preceding),
                                        std::make_tuple(std::move(last)))};
    }

    auto operator|(TaskFinish) && {
        return Task<Steps..., Fn>{std::tuple_cat(
            std::move(preceding), std::make_tuple(std::move(last)))};
    }
};

struct TaskStart {
    auto operator|(TaskFinish) && { return Task<>{{}}; }

    template <std::invocable Fn>
    auto operator|(Fn&& fn) && {
        return TaskBuilder<Fn>{.last = std::forward<Fn>(fn)};
    }
};

constexpr auto task() { return TaskStart(); }

}  // namespace plai::sched
