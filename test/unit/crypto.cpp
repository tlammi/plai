#include <gtest/gtest.h>

#include <plai/crypto.hpp>
#include <plai/util/str.hpp>

TEST(Sha256, Empty) {
    auto digest = plai::crypto::sha256("");
    auto str = plai::to_hex_str(digest);
    ASSERT_EQ(
        str,
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(Sha256, Foo) {
    auto digest = plai::crypto::sha256("foo");
    auto str = plai::to_hex_str(digest);
    ASSERT_EQ(
        str,
        "2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae");
}

TEST(Sha256, Bar) {
    auto digest = plai::crypto::sha256("bar");
    auto str = plai::to_hex_str(digest);
    ASSERT_EQ(
        str,
        "fcde2b2edba56bf408601fb721fe9b5c338d10ee429ea04fae5511b68fbf8fb9");
}

TEST(Sha256, Long) {
    std::string_view input =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    auto digest = plai::crypto::sha256(input);
    auto str = plai::to_hex_str(digest);
    ASSERT_EQ(
        str,
        "450f6ace7b48890a36b71578c597592f7db6ab57b6c0d292f0a638eac1a201b0");
}

TEST(Sha256, Binary) {
    // NOLINTNEXTLINE
    auto input = std::vector<uint8_t>{1, 2, 3, 4, 5};
    auto digest = plai::crypto::sha256(input);
    auto str = plai::to_hex_str(digest);
    ASSERT_EQ(
        str,
        "74f81fe167d99b4cb41d6d0ccda82278caee9f3e2f25d5e5a3936ff3dcec60d0");
}

