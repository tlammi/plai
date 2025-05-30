#include <gtest/gtest.h>

#include <plai/vec.hpp>

// NOLINTBEGIN(*magic-numbers*)

TEST(Vec, BoundToX) {
    auto vec = plai::Vec(2, 1);
    const auto bound = plai::Vec(100, 100);

    vec.scale_to(bound);

    ASSERT_EQ(vec.x, 100);
    ASSERT_EQ(vec.y, 50);
}

TEST(Vec, BoundToY) {
    auto vec = plai::Vec(1, 2);
    const auto bound = plai::Vec(100, 100);
    vec.scale_to(bound);
    ASSERT_EQ(vec.x, 50);
    ASSERT_EQ(vec.y, 100);
}

// NOLINTEND(*magic-numbers*)
