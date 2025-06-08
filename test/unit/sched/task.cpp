#include <gtest/gtest.h>

#include <plai/sched/task.hpp>

namespace sched = plai::sched;

TEST(Create, Null) {
    auto task = sched::task() | sched::task_finish();
    ASSERT_EQ(task.step_count(), 0);
}

TEST(Create, One) {
    auto task = sched::task() | [] {} | sched::task_finish();
    ASSERT_EQ(task.step_count(), 1);
}

TEST(Create, ChainVoid) {
    auto task = sched::task() | [] {} | [] {} | sched::task_finish();
    ASSERT_EQ(task.step_count(), 2);
}

TEST(Create, ChainVal) {
    auto task = sched::task() | [] { return 1; } | [](int i) { (void)i; } |
                sched::task_finish();
    ASSERT_EQ(task.step_count(), 2);
}

TEST(Run, One) {
    size_t counter = 0;
    auto task = sched::task() | [&] { ++counter; } | sched::task_finish();
    task.launch().run();
    ASSERT_EQ(counter, 1);
}

TEST(Run, MultipleVoid) {
    size_t counter = 0;
    auto task = sched::task() | [&] { ++counter; } | [&] { counter *= 2; } |
                sched::task_finish();
    task.launch().run();
    ASSERT_EQ(counter, 2);
}

TEST(Step, One) {
    size_t counter = 0;
    auto task = sched::task() | [&] { ++counter; } | sched::task_finish();
    auto st = task.launch();
    ASSERT_FALSE(st.done());
    st();
    ASSERT_TRUE(st.done());
    ASSERT_EQ(counter, 1);
}

TEST(Step, Many) {
    size_t counter = 0;
    auto task = sched::task() | [&] { return counter + 1; } |
                [](size_t s) { return s * 2; } |
                [&](size_t s) { counter = s; } | sched::task_finish();
    auto st = task.launch();
    size_t iterations = 0;
    while (!st.done()) {
        ++iterations;
        st();
    }
    ASSERT_EQ(counter, 2);
    ASSERT_EQ(iterations, 3);
}
