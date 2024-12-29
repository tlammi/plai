#include <gtest/gtest.h>

#include <plai/util/str.hpp>

TEST(ToHexStr, Empty) {
    std::vector<uint8_t> v{};
    auto res = plai::to_hex_str(v);
    ASSERT_TRUE(res.empty());
}

TEST(ToHexStr, One) {
    std::vector<uint8_t> v{0x01};
    auto res = plai::to_hex_str(v);
    ASSERT_EQ(res, "01");
}

TEST(ToHexStr, Max) {
    auto v = std::vector<uint8_t>{0xff};  // NOLINT
    auto res = plai::to_hex_str(v);
    ASSERT_EQ(res, "ff");
}

TEST(ToHexStr, Many) {
    auto v = std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04,
                                  0x05, 0x60, 0x61, 0x62};  // NOLINT
    auto res = plai::to_hex_str(v);

    ASSERT_EQ(res, "0102030405606162");
}
