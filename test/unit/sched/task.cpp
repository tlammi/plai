#include <gtest/gtest.h>

#include <plai/sched/executor.hpp>
#include <plai/sched/task.hpp>

namespace sched = plai::sched;
using namespace std::literals;

TEST(Create, Null) {
    auto ctx = sched::IoContext();
    auto task = sched::task() | sched::executor(ctx) | sched::task_finish();
    ASSERT_EQ(task.step_count(), 0);
}

TEST(Create, One) {
    auto ctx = sched::IoContext();
    auto task =
        sched::task() | sched::executor(ctx) | [] {} | sched::task_finish();
    ASSERT_EQ(task.step_count(), 1);
}

TEST(Create, ChainVoid) {
    auto ctx = sched::IoContext();
    auto task = sched::task() | sched::executor(ctx) | [] {} | [] {} |
                sched::task_finish();
    ASSERT_EQ(task.step_count(), 2);
}

TEST(Create, ChainVal) {
    auto ctx = sched::IoContext();
    auto task = sched::task() | sched::executor(ctx) | [] { return 1; } |
                [](int i) { (void)i; } | sched::task_finish();
    ASSERT_EQ(task.step_count(), 2);
}

TEST(Run, One) {
    size_t counter = 0;
    auto ctx = sched::IoContext();
    auto task = sched::task() | sched::executor(ctx) | [&] { ++counter; } |
                sched::task_finish();
    task.post();
    ctx.run();
    ASSERT_EQ(counter, 1);
}

TEST(Run, MultipleVoid) {
    auto ctx = sched::IoContext();
    size_t counter = 0;
    auto task = sched::task() | sched::executor(ctx) | [&] { ++counter; } |
                [&] { counter *= 2; } | sched::task_finish();
    task.post();
    ctx.run();
    ASSERT_EQ(counter, 2);
}

TEST(Step, One) {
    auto ctx = sched::IoContext();
    size_t counter = 0;
    auto task = sched::task() | sched::executor(ctx) | [&] { ++counter; } |
                sched::task_finish();
    task.post();
    ASSERT_TRUE(ctx.poll_one());
    ASSERT_EQ(counter, 1);
}

TEST(Step, Many) {
    auto ctx = sched::IoContext();
    size_t counter = 0;
    auto task = sched::task() | sched::executor(ctx) |
                [&] { return counter + 1; } | [](size_t s) { return s * 2; } |
                [&](size_t s) { counter = s; } | sched::task_finish();
    task.post();
    size_t iterations = 0;
    while (ctx.poll_one()) { ++iterations; }
    ASSERT_EQ(counter, 2);
    ASSERT_EQ(iterations, 3);
}

TEST(PeriodicTask, Create) {
    auto ctx = sched::IoContext();
    auto task = sched::task() | sched::executor(ctx) | sched::period(10ms) |
                [] {} | sched::task_finish();
    ASSERT_EQ(task.step_count(), 1);
}

TEST(PeriodicTask, Run) {
    auto ctx = sched::IoContext();
    size_t counter = 0;
    static constexpr auto count = 100;
    auto task = sched::task() | sched::executor(ctx) | sched::period(1ms) |
                [&] { ++counter; } | sched::task_finish();
    while (counter < count) { ctx.poll(); }
}
