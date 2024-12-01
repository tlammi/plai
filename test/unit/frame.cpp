#include <gtest/gtest.h>

#include <plai/media/frame.hpp>

using plai::media::Frame;
TEST(DefaultFrame, Empty) {
    auto frm = Frame();
    ASSERT_EQ(frm.width(), 0);
    ASSERT_EQ(frm.height(), 0);
}
