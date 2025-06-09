#pragma once

#include <memory>
#include <plai/sched/post_chain.hpp>
#include <utility>

namespace plai::sched {

class Task {
    template <class, class...>
    friend class TaskBuilder;
    template <class, class...>
    friend class TaskImpl;

    class Impl {
     public:
        virtual ~Impl() = default;

        virtual void post() = 0;
        virtual size_t step_count() const noexcept = 0;
    };

 public:
    void post() { m_impl->post(); }
    size_t step_count() const noexcept { return m_impl->step_count(); }

 private:
    explicit Task(std::unique_ptr<Impl> impl) noexcept
        : m_impl(std::move(impl)) {}

    std::unique_ptr<Impl> m_impl{};
};

template <class Exec, class... Fns>
class TaskImpl : public Task::Impl {
 public:
    TaskImpl(Exec exec, std::tuple<Fns...> fns) noexcept
        : m_exec(std::move(exec)), m_fns(std::move(fns)) {}

    void post() override {
        if constexpr (sizeof...(Fns)) {
            std::apply(
                [&](auto&&... args) {
                    post_chain(m_exec, std::forward<decltype(args)>(args)...);
                },
                m_fns);
        }
    }

    size_t step_count() const noexcept override { return sizeof...(Fns); }

 private:
    Exec m_exec;
    std::tuple<Fns...> m_fns;
};

namespace detail {

struct TaskFinish {};

template <class Exec>
struct Executor {
    Exec value;
};

}  // namespace detail

template <class Exec>
constexpr auto executor(Exec&& exec) {
    if constexpr (requires { std::declval<Exec>().get_executor(); }) {
        return detail::Executor{exec.get_executor()};
    } else {
        return detail::Executor{std::forward<Exec>(exec)};
    }
}

constexpr detail::TaskFinish task_finish() noexcept { return {}; }

template <class Exec, class... Fns>
class TaskBuilder {
 public:
    explicit TaskBuilder(Exec exec) noexcept
        requires(sizeof...(Fns) == 0)
        : m_exec(std::move(exec)) {}

    TaskBuilder(Exec exec, std::tuple<Fns...> fns) noexcept
        : m_exec(std::move(exec)), m_fns(std::move(fns)) {}

    template <class Fn>
    TaskBuilder<Exec, Fns..., Fn> operator|(Fn&& fn) && {
        return {std::move(m_exec),
                std::tuple_cat(std::move(m_fns),
                               std::make_tuple(std::forward<Fn>(fn)))};
    }

    template <class E>
    TaskBuilder<E, Fns...> operator|(detail::Executor<E> e) && {
        return {std::move(e.value), std::move(m_fns)};
    }

    Task operator|(detail::TaskFinish) && {
        static_assert(!std::same_as<Exec, std::nullptr_t>,
                      "Executor is not set");
        return Task(std::make_unique<TaskImpl<Exec, Fns...>>(std::move(m_exec),
                                                             std::move(m_fns)));
    }

 private:
    Exec m_exec;
    std::tuple<Fns...> m_fns{};
};

constexpr TaskBuilder<std::nullptr_t> task() {
    return TaskBuilder<std::nullptr_t>{nullptr};
}

}  // namespace plai::sched
