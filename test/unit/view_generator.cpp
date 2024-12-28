#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <plai/util/view_generator.hpp>

using plai::view_generator;
using plai::ViewGenerator;

TEST(Gen, EmptyRange) {
    auto gen = ViewGenerator([] -> std::optional<int> { return std::nullopt; });
    auto vec = gen | std::ranges::to<std::vector>();
    ASSERT_TRUE(vec.empty());
}

TEST(Gen, OneValue) {
    std::vector<int> in{1};
    std::span<int> spn(in);
    auto gen = view_generator([&] -> std::optional<int> {
        if (!spn.empty()) {
            auto v = spn.front();
            spn = spn.subspan(1);
            return v;
        }
        return std::nullopt;
    });

    auto vec = gen | std::ranges::to<std::vector>();
    ASSERT_EQ(vec.size(), 1);
    ASSERT_EQ(vec.front(), 1);
}

TEST(Gen, MultipleValues) {
    std::vector<int> in{10, 20, 30, 40};  // NOLINT
    auto span = std::span<int>(in);
    auto gen = view_generator([&] -> std::optional<int> {
        if (!span.empty()) {
            auto v = span.front();
            span = span.subspan(1);
            return v;
        }
        return std::nullopt;
    });
    auto vec = gen | std::ranges::to<std::vector>();
    ASSERT_THAT(vec, testing::ElementsAre(10, 20, 30, 40));
}

