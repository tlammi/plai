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
