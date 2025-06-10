#include <plai/sched/task.hpp>
#include <print>

using namespace std::literals;

int main() {
    static constexpr auto period = 100ms;
    size_t measurements = 0;
    plai::logs::init(plai::logs::Level::Trace);
    plai::Duration max_period = plai::Duration::min();
    plai::TimePoint prev = plai::TimePoint();
    plai::FloatDuration avg_period = plai::FloatDuration::zero();

    auto ctx = plai::sched::IoContext();
    auto task = plai::sched::task() | plai::sched::executor(ctx) |
                plai::sched::period(period) |
                [&] {
                    ++measurements;
                    auto now = plai::Clock::now();
                    auto delay = now - prev;
                    prev = now;
                    if (avg_period == plai::FloatDuration::zero())
                        avg_period = period;
                    else
                        avg_period =
                            delay / measurements +
                            (double(measurements - 1) / double(measurements)) *
                                avg_period;
                    max_period = std::max(max_period, delay);
                    std::println("last: {}, max: {}, avg: {}", delay,
                                 max_period, avg_period);
                } |
                plai::sched::task_finish();
    prev = plai::Clock::now();
    ctx.run();
}
