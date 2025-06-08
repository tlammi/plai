#include <gtest/gtest.h>

#include <plai/buffer.hpp>

constexpr auto kilo = 1024;

TEST(Ctor, Default) { auto b = plai::Buffer(); }

TEST(Ctor, Size) { auto b = plai::Buffer(kilo); }

TEST(Ctor, Value) {
    auto b =
        plai::Buffer(std::in_place_type<std::unique_ptr<int>>, new int{100});
    ASSERT_EQ(**b.get<std::unique_ptr<int>>(), 100);
}

TEST(Mutate, IntToInt) {
    auto b = plai::make_buffer<int>(1);
    b.mutate([](int i) { return i + 1; });
    ASSERT_EQ(*b.get<int>(), 2);
}

TEST(Mutate, Clear) {
    auto b = plai::make_buffer<int>(2);
    int res = 0;
    b.mutate([&](int i) { res = i; });
    ASSERT_EQ(res, 2);
    ASSERT_FALSE(b);
}
