#include <gtest/gtest.h>

#include <plai/frac.hpp>

using namespace plai::literals;

TEST(Frac, Init) {
    auto f = "3/4"_frac;
    ASSERT_EQ(f.num(), 3);
    ASSERT_EQ(f.den(), 4);
}

TEST(Frac, Reciprocal) {
    auto f = "3/4"_frac.reciprocal();
    ASSERT_EQ(f.num(), 4);
    ASSERT_EQ(f.den(), 3);
}

TEST(Frac, ToFloat) {
    auto f = "3/4"_frac;
    auto d = double(f);
    ASSERT_EQ(d, .75);
}
