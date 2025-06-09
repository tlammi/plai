#include <gtest/gtest.h>

#include <plai/sched/executor.hpp>
#include <plai/sched/post_chain.hpp>

namespace s = plai::sched;

TEST(Chain, One) {
    size_t counter = 0;
    auto ctx = s::IoContext();
    s::post_chain(ctx, [&] { ++counter; });
    ctx.run();
    ASSERT_EQ(counter, 1);
}

TEST(Chain, MultipleVoid) {
    size_t counter = 0;
    auto ctx = s::IoContext();
    auto inc = [&] { ++counter; };
    auto mul = [&] { counter *= 2; };
    s::post_chain(ctx, inc, mul, inc);
    ctx.run();
    ASSERT_EQ(counter, 3);
}

TEST(Chain, ChainValues) {
    auto start = [] { return 1; };
    auto inc = [](size_t i) { return i + 1; };
    auto mul = [](size_t i) { return i * 2; };
    size_t res = 0;
    auto finish = [&](size_t i) { res = i; };

    auto ctx = s::IoContext();
    s::post_chain(ctx, start, inc, inc, mul, inc, finish);
    ctx.run();
    ASSERT_EQ(res, 7);
}
