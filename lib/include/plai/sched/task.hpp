#pragma once

#include <memory>
#include <plai/logs/logs.hpp>
#include <plai/sched/executor.hpp>
#include <plai/sched/post_chain.hpp>
#include <plai/time.hpp>
#include <utility>

namespace plai::sched {
namespace detail {

class TaskImpl {
 public:
    virtual ~TaskImpl() = default;

    virtual void post() = 0;
    virtual size_t step_count() const noexcept = 0;
};
struct task_tag_t {};
constexpr auto task_tag = task_tag_t{};
}  // namespace detail

class Task {
 public:
    constexpr Task() noexcept = default;
    Task(detail::task_tag_t, std::unique_ptr<detail::TaskImpl> impl) noexcept
        : m_impl(std::move(impl)) {}
    void post() { m_impl->post(); }
    size_t step_count() const noexcept { return m_impl->step_count(); }

 private:
    std::unique_ptr<detail::TaskImpl> m_impl{};
};

class PeriodicTask {
    struct Ctx {
        Timer timer;
        TimePoint start{};
        std::atomic<Duration> period{};
        detail::TaskImpl* impl{};
    };

 public:
    template <class Exec>
    PeriodicTask(detail::task_tag_t, Exec exec, Duration period,
                 std::unique_ptr<detail::TaskImpl> impl) noexcept
        : m_impl(std::move(impl)),
          m_ctx(new Ctx{.timer = Timer(std::move(exec)),
                        .period = std::move(period),
                        .impl = m_impl.get()}) {
        loop(m_ctx.get());
    }

    size_t step_count() const noexcept { return m_impl->step_count(); }

    void set_period(Duration period) {
        m_ctx->period.store(period, std::memory_order_relaxed);
    }

 private:
    static void loop(Ctx* ctx) {
        auto now = Clock::now();
        auto dur = now - ctx->start;
        auto period = ctx->period.load(std::memory_order_relaxed);
        auto next = ctx->start + period;
        if (dur > period) {
            PLAI_WARN("PeriodicTask overrun");
            next = now;
        }
        ctx->start = next;
        ctx->timer.expires_at(next);
        ctx->timer.async_wait([ctx = std::move(ctx)](auto ec) mutable {
            if (ec) return;
            ctx->impl->post();
            loop(ctx);
        });
    }

    std::unique_ptr<detail::TaskImpl> m_impl{};
    std::unique_ptr<Ctx> m_ctx{};
};

template <class Exec, class... Fns>
class TaskImpl : public detail::TaskImpl {
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

struct Period {
    Duration value;
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

constexpr auto period(Duration d) { return detail::Period{d}; }

constexpr detail::TaskFinish task_finish() noexcept { return {}; }

template <class Exec, class Period, class... Fns>
class TaskBuilder {
 public:
    explicit TaskBuilder(Exec exec) noexcept
        requires(sizeof...(Fns) == 0)
        : m_exec(std::move(exec)) {}

    TaskBuilder(Exec exec, Period period, std::tuple<Fns...> fns) noexcept
        : m_exec(std::move(exec)),
          m_period(std::move(period)),
          m_fns(std::move(fns)) {}

    template <class Fn>
    TaskBuilder<Exec, Period, Fns..., Fn> operator|(Fn&& fn) && {
        return {std::move(m_exec), std::move(m_period),
                std::tuple_cat(std::move(m_fns),
                               std::make_tuple(std::forward<Fn>(fn)))};
    }

    template <class E>
    TaskBuilder<E, Period, Fns...> operator|(detail::Executor<E> e) && {
        return {std::move(e.value), std::move(m_period), std::move(m_fns)};
    }

    TaskBuilder<Exec, Duration, Fns...> operator|(detail::Period p) && {
        return {std::move(m_exec), std::move(p.value), std::move(m_fns)};
    }

    auto operator|(detail::TaskFinish) && {
        static_assert(!std::same_as<Exec, std::nullptr_t>,
                      "Executor is not set");
        if constexpr (std::same_as<Period, std::nullptr_t>) {
            return Task(detail::task_tag,
                        std::make_unique<TaskImpl<Exec, Fns...>>(
                            std::move(m_exec), std::move(m_fns)));
        } else {
            return PeriodicTask(detail::task_tag, m_exec, m_period,
                                std::make_unique<TaskImpl<Exec, Fns...>>(
                                    m_exec, std::move(m_fns)));
        }
    }

 private:
    Exec m_exec;
    Period m_period{};
    std::tuple<Fns...> m_fns{};
};

constexpr auto task() {
    return TaskBuilder<std::nullptr_t, std::nullptr_t>{nullptr};
}

}  // namespace plai::sched
