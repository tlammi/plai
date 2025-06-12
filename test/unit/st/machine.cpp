#include <gtest/gtest.h>

#include <plai/st/state_machine.hpp>

namespace st = plai::st;
enum class StA {
    Init,
    A,
    B,
    Done,
};

struct StepThrough {
    using state_type = StA;
    using enum state_type;

    state_type step(st::tag_t<Init>) { return A; }
    state_type step(st::tag_t<A>) { return B; }
    state_type step(st::tag_t<B>) { return Done; }
};

TEST(Sm, Init) {
    using enum StA;
    auto sm = st::StateMachine<StepThrough>();
    ASSERT_EQ(sm.state(), Init);
    ASSERT_FALSE(sm.done());
}

TEST(Sm, Step) {
    using enum StA;
    auto sm = st::StateMachine<StepThrough>();
    sm();
    ASSERT_EQ(sm.state(), A);
    ASSERT_FALSE(sm.done());
}

TEST(Sm, StepEnd) {
    using enum StA;
    auto sm = st::StateMachine<StepThrough>();
    size_t count = 0;
    while (!sm.done()) {
        sm();
        ++count;
    }
    ASSERT_EQ(count, 3);
}

enum class SimpleSt {
    Run,
    Done,
};

struct Stateful {
    bool* run;
    using enum SimpleSt;
    using state_type = SimpleSt;

    state_type step(st::tag_t<Run>) {
        if (*run) return Run;
        return Done;
    }
};

TEST(Sm, Stateful) {
    using enum SimpleSt;
    bool run = true;
    auto sm = st::StateMachine<Stateful>({.run = &run});
    sm();
    ASSERT_EQ(sm.state(), Run);
    sm();
    ASSERT_EQ(sm.state(), Run);
    sm();
    ASSERT_EQ(sm.state(), Run);
    run = false;
    sm();
    ASSERT_TRUE(sm.done());
}

struct Params {
    int value{};
    using enum SimpleSt;
    using state_type = SimpleSt;

    SimpleSt step(st::tag_t<Run>, int v) {
        value = v;
        if (!value) return Done;
        return Run;
    }
};

TEST(Sm, Param) {
    using enum SimpleSt;
    auto sm = st::StateMachine<Params>();
    sm(1);
    ASSERT_EQ(sm->value, 1);
    sm(2);
    ASSERT_EQ(sm->value, 2);
    sm(0);
    ASSERT_TRUE(sm.done());
}
