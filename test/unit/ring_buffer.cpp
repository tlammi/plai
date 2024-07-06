#include <gtest/gtest.h>

#include <plai/util/ring_buffer.hpp>

using plai::RingBuffer;
TEST(Ctor, Default) {
  auto rb = RingBuffer<int>(100);
  ASSERT_EQ(rb.size(), 0);
  ASSERT_EQ(rb.capacity(), 100);
}

TEST(PushPop, One) {
  auto rb = RingBuffer<int>(100);
  rb.emplace(100);
  ASSERT_EQ(rb.size(), 1);
  auto i = rb.pop();
  ASSERT_EQ(i, 100);
  ASSERT_EQ(rb.size(), 0);
}

TEST(PushPop, Three) {
  auto rb = RingBuffer<int>(100);
  rb.emplace(1);
  rb.emplace(2);
  rb.emplace(3);
  ASSERT_EQ(rb.size(), 3);

  ASSERT_EQ(rb.pop(), 1);
  ASSERT_EQ(rb.pop(), 2);
  ASSERT_EQ(rb.pop(), 3);
  ASSERT_EQ(rb.size(), 0);
}

TEST(PushPop, TryOne) {
  auto rb = RingBuffer<int>(3);
  auto emplaced = rb.try_emplace(1);
  ASSERT_EQ(rb.size(), 1);
  ASSERT_TRUE(emplaced);
  auto res = rb.try_pop();
  ASSERT_TRUE(res);
  ASSERT_EQ(*res, 1);
}

TEST(Push, OverLimit) {
  auto rb = RingBuffer<int>(2);
  ASSERT_TRUE(rb.try_emplace(1));
  ASSERT_TRUE(rb.try_emplace(1));
  ASSERT_FALSE(rb.try_emplace(1));
}

TEST(Pop, OverLimit) {
  auto rb = RingBuffer<int>(2);
  ASSERT_FALSE(rb.try_pop());
  rb.emplace(1);
  ASSERT_TRUE(rb.try_pop());
  ASSERT_FALSE(rb.try_pop());
}
