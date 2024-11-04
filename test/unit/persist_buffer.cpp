#include <gtest/gtest.h>

#include <limits>
#include <plai/persist_buffer.hpp>
#include <thread>

TEST(Init, Capacity) {
    auto p = plai::PersistBuffer<int>(1);
    ASSERT_EQ(p.capacity(), 1);
}

TEST(PushPop, Simple) {
    auto p = plai::PersistBuffer<size_t>(3);
    static constexpr size_t count = 100;
    for (size_t i = 0; i < count; ++i) {
        auto prev = p.push(i);
        auto res = p.pop(prev);
        ASSERT_EQ(res, i);
    }
}

TEST(PushPop, Multithread) {
    static constexpr size_t count = 1000;
    auto p = plai::PersistBuffer<size_t>(3);
    auto t = std::jthread([&] {
        for (size_t i = 1; i < count; ++i) { p.push(i); }
        p.push(std::numeric_limits<size_t>::max());
    });

    size_t prev = 0;
    size_t res = 0;
    while (true) {
        res = p.pop(std::move(res));
        if (res == std::numeric_limits<size_t>::max()) break;
        ASSERT_EQ(res, prev + 1);
        prev = res;
    }
}

TEST(PushPop, MoveOnly) {
    static constexpr size_t count = 1000;
    auto p = plai::PersistBuffer<std::unique_ptr<size_t>>(
        3, plai::factory, [] { return std::make_unique<size_t>(); });

    auto input = std::unique_ptr<size_t>(new size_t(0));
    auto output = std::make_unique<size_t>();
    for (size_t i = 0; i < count; ++i) {
        *input = i;
        input = p.push(std::move(input));
        output = p.pop(std::move(output));
        ASSERT_EQ(*output, i);
    }
}

