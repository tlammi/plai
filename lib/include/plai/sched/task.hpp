#pragma once

#include <concepts>
#include <cstddef>
#include <plai/sched/detail/step.hpp>
#include <plai/sched/executor.hpp>
#include <plai/type_traits.hpp>
#include <plai/util/tuple.hpp>
#include <plai/virtual.hpp>
#include <tuple>

namespace plai::sched {

namespace detail {

template <class T>
struct single_param_type;

template <class R, class P>
struct single_param_type<R(P)> {
    using type = P;
};

template <size_t Idx, class Tupl>
void foreach_tuple(Tupl& tupl) {
    if constexpr (Idx < std::tuple_size_v<std::remove_cvref_t<Tupl>>) {
        std::get<Idx>(tupl)();
        foreach_tuple<Idx + 1>(tupl);
    }
}

template <class... Fns>
std::tuple<Step<Fns>...> wrap_steps(Buffer& buf, std::tuple<Fns...> fns) {
    return std::apply(
        [&](auto&&... args) {
            return std::tuple<Step<Fns>...>{
                Step(buf, std::forward<decltype(args)>(args))...};
        },
        std::move(fns));
}
}  // namespace detail

template <class R>
class TaskState;

template <class Proto>
class Task;

template <class R, class... Ps>
class Task<R(Ps...)> {
    template <class...>
    friend class TaskImpl;
    friend class TaskState<R>;
    class Impl {
        friend class TaskState<R>;

     public:
        virtual ~Impl() = default;
        virtual TaskState<R> launch(Ps... ps) = 0;
        virtual size_t step_count() const noexcept = 0;

     private:
        virtual void run_step(size_t idx) = 0;
    };

 public:
    explicit Task(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

    auto launch(Ps... ps) { return m_impl->launch(); }

    auto step_count() const noexcept { return m_impl->step_count(); }

 private:
    std::unique_ptr<Impl> m_impl{};
};

template <>
class TaskState<void> {
 public:
    explicit TaskState(Task<void()>::Impl& impl) : m_impl(&impl) {}

    bool done() const noexcept { return m_idx == m_count; }

    void step() {
        m_impl->run_step(m_idx);
        ++m_idx;
    }

    void operator()() { step(); }

    void run() {
        while (!done()) step();
    }

 private:
    Task<void()>::Impl* m_impl;
    size_t m_idx{};
    size_t m_count{m_impl->step_count()};
};

template <class... Steps>
class TaskImpl final : public Task<void()>::Impl {
    using Args = std::tuple<Steps...>;
    using Tuple = std::tuple<detail::Step<Steps>...>;

 public:
    explicit TaskImpl(Args steps) noexcept
        : m_steps(detail::wrap_steps(m_buf, std::move(steps))) {}

    TaskImpl(const TaskImpl&) = delete;
    TaskImpl& operator=(const TaskImpl&) = delete;

    TaskImpl(TaskImpl&&) = delete;
    TaskImpl& operator=(TaskImpl&&) = delete;

    ~TaskImpl() override = default;

    constexpr size_t step_count() const noexcept override {
        return std::tuple_size_v<Tuple>;
    }

    TaskState<void> launch() override { return TaskState<void>{*this}; }

 private:
    void run_step(size_t idx) override {
        get_nth<detail::AnyStep>(m_steps, idx)();
    }
    Buffer m_buf{};
    Tuple m_steps{};
};

namespace detail {
template <class Type, class... Ps>
Task<void()> make_task(Ps&&... ps) {
    return Task<void()>(
        std::unique_ptr<Type>(new Type(std::forward<Ps>(ps)...)));
}
}  // namespace detail

struct TaskFinish {};
constexpr auto task_finish() { return TaskFinish{}; }

template <class Fn, class... Steps>
struct TaskBuilder {
    Fn last;
    std::tuple<Steps...> preceding;

    template <class Fn2>
    auto operator|(Fn2&& fn) && {
        if constexpr (std::is_void_v<invoke_result_t<Fn>>) {
            static_assert(std::invocable<Fn2>);
            return TaskBuilder<Fn2, Steps..., Fn>{
                .last = std::forward<Fn2>(fn),
                .preceding = std::tuple_cat(std::move(preceding),
                                            std::make_tuple(std::move(last)))};
        } else {
            // Needs to be exactly the same type since accessed from a Buffer
            static_assert(std::same_as<typename detail::single_param_type<
                                           prototype_of_t<Fn2>>::type,
                                       invoke_result_t<Fn>>);
            return TaskBuilder<Fn2, Steps..., Fn>{
                .last = std::forward<Fn2>(fn),
                .preceding = std::tuple_cat(std::move(preceding),
                                            std::make_tuple(std::move(last)))};
        }
    }

    auto operator|(TaskFinish) && {
        return detail::make_task<TaskImpl<Steps..., Fn>>(std::tuple_cat(
            std::move(preceding), std::make_tuple(std::move(last))));
    }
};

struct TaskStart {
    auto operator|(TaskFinish) && {
        return detail::make_task<TaskImpl<>>(std::tuple<>());
    }

    template <std::invocable Fn>
    auto operator|(Fn&& fn) && {
        return TaskBuilder<Fn>{.last = std::forward<Fn>(fn)};
    }
};

constexpr auto task() { return TaskStart(); }

}  // namespace plai::sched
