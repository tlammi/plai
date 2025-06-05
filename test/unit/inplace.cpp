#include <gtest/gtest.h>

#include <plai/inplace.hpp>

TEST(Ctor, DefaultTrivial) {
    auto i = plai::Inplace<int, sizeof(int)>();
    ASSERT_FALSE(i);
}

class Base {
 public:
    constexpr virtual ~Base() = default;

    virtual int foo() const noexcept { return 0; }
};

TEST(Ctor, DefaultVirtual) {
    class Foo : public Base {};
    auto i = plai::Inplace<Foo, sizeof(Foo)>();
    ASSERT_FALSE(i);
}

TEST(Ctor, ValueTrivial) {
    auto i = plai::Inplace<int, sizeof(int)>(std::in_place_type<int>, 2);
    ASSERT_TRUE(i);
    ASSERT_EQ(*i, 2);
}

TEST(Ctor, ValueVirtual) {
    class Foo : public Base {
     public:
        int foo() const noexcept override { return 1; }
    };

    auto i = plai::Inplace<Foo, sizeof(Foo)>(std::in_place_type<Foo>);
    ASSERT_TRUE(i);
    ASSERT_EQ(i->foo(), 1);
}

TEST(Dtor, Normal) {
    // NOLINTNEXTLINE
    class Foo : public Base {
     public:
        explicit Foo(int* counter) : m_counter(counter) {}

        ~Foo() override {
            if (m_counter) ++*m_counter;
        }

     private:
        int* m_counter;
    };

    int counter = 0;

    {
        auto i = plai::make_inplace<Base, Foo>(&counter);
        ASSERT_EQ(counter, 0);
    }
    ASSERT_EQ(counter, 1);
}

TEST(Dtor, Move) {
    // NOLINTNEXTLINE
    class Foo : public Base {
     public:
        Foo(int* counter) : m_counter(counter) {}
        Foo(const Foo&) = delete;
        Foo& operator=(const Foo&) = delete;

        Foo(Foo&& other) noexcept
            : m_counter(std::exchange(other.m_counter, nullptr)) {}

        Foo& operator=(Foo&& other) noexcept {
            auto tmp = std::move(*this);
            m_counter = std::exchange(other.m_counter, nullptr);
            return *this;
        }

        ~Foo() override {
            if (m_counter) ++*m_counter;
        }

     private:
        int* m_counter;
    };

    int counter = 0;
    {
        auto i = plai::make_inplace<Base, Foo>(&counter);
        auto j = std::move(i);
        ASSERT_EQ(counter, 0);
        i = std::move(j);
        ASSERT_EQ(counter, 0);
    }
    ASSERT_EQ(counter, 1);
}
