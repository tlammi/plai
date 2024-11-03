#include <gtest/gtest.h>

#include <plai/queue.hpp>

TEST(Init, Default) {
    auto q = plai::Queue<int>();
    ASSERT_TRUE(q.empty());
    ASSERT_EQ(q.size(), 0);
}

TEST(Init, Values) {
    auto q = plai::Queue<int>{std::in_place, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    ASSERT_EQ(q.size(), 10);
    ASSERT_FALSE(q.empty());
}

TEST(Access, Front) {
    auto q = plai::Queue<int>{std::in_place, 1};
    ASSERT_EQ(q.front(), 1);
}

TEST(Access, Back) {
    auto q = plai::Queue<int>(std::in_place, 1, 2);
    ASSERT_EQ(q.back(), 2);
}

struct Incrementer {
    Incrementer(int* p, int val) : p(p), val(val) {}

    Incrementer(const Incrementer&) = delete;
    Incrementer& operator=(const Incrementer&) = delete;

    Incrementer(Incrementer&& other) noexcept
        : p(std::exchange(other.p, nullptr)), val(other.val) {}

    Incrementer& operator=(Incrementer&& other) noexcept {
        auto tmp = Incrementer(std::move(other));
        std::swap(p, tmp.p);
        std::swap(val, tmp.val);
        return *this;
    }

    ~Incrementer() {
        if (p) *p += val;
    }
    int* p;
    int val;
};

TEST(Cleanup, One) {
    int i = 0;
    {
        auto q = plai::Queue<Incrementer>(std::in_place, Incrementer(&i, 1));
        ASSERT_EQ(i, 0);
    }
    ASSERT_EQ(i, 1);
}

TEST(Cleanup, Multiple) {
    int i = 0;
    {
        auto q = plai::Queue<Incrementer>(std::in_place, Incrementer(&i, 1),
                                          Incrementer(&i, 2));
        ASSERT_EQ(i, 0);
    }
    ASSERT_EQ(i, 3);
}

TEST(Cleanup, AfterMove) {
    int i = 0;
    {
        auto q1 = plai::Queue<Incrementer>();
        {
            auto q2 =
                plai::Queue<Incrementer>(std::in_place, Incrementer(&i, 1));
            ASSERT_EQ(i, 0);
            q1 = std::move(q2);
            ASSERT_EQ(i, 0);
        }
        ASSERT_EQ(i, 0);
    }
    ASSERT_EQ(i, 1);
}

TEST(Cleanup, LargeCount) {
    static constexpr auto large_number = 10000;
    int integer = 0;
    {
        auto q = plai::Queue<Incrementer>();
        for (size_t i = 0; i < large_number; ++i) {
            q.emplace_back(&integer, 1);
        }
        ASSERT_EQ(integer, 0);
    }
    ASSERT_EQ(integer, large_number);
}

