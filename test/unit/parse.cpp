#include <gtest/gtest.h>

#include <plai/util/parse.hpp>

TEST(ToInt, Zero) {
    auto i = plai::to_number<int>("0");
    ASSERT_TRUE(i);
    ASSERT_EQ(*i, 0);
}

TEST(ToInt, Num) {
    auto i = plai::to_number<int>("12345");
    ASSERT_TRUE(i);
    ASSERT_EQ(*i, 12345);
}

TEST(ToInt, Err) {
    auto i = plai::to_number<int>("foo");
    ASSERT_FALSE(i);
}
