#include <gtest/gtest.h>

#include <plai/net/http/target.hpp>

using plai::net::http::parse_target;

TEST(Parse, NoWildcards) {
    auto res = parse_target("/foo", "/foo");
    ASSERT_TRUE(res);
    ASSERT_EQ(*res, "/foo");
}

TEST(Parse, SingleWildcard) {
    auto res = parse_target("/foo/{param}/bar", "/foo/asd/bar");
    ASSERT_TRUE(res);
    ASSERT_EQ(*res, "/foo/asd/bar");
    ASSERT_EQ(res->at("param"), "asd");
}

TEST(Parse, MultiWildcard) {
    auto res = parse_target("/{a}/{b}/c/{d}/e/{f}", "/aa/bb/c/dd/e/ff");
    ASSERT_TRUE(res);
    ASSERT_EQ(*res, "/aa/bb/c/dd/e/ff");
    ASSERT_EQ(res->params().size(), 4);
    ASSERT_EQ(res->at("a"), "aa");
    ASSERT_EQ(res->at("b"), "bb");
    ASSERT_EQ(res->at("d"), "dd");
    ASSERT_EQ(res->at("f"), "ff");
}

TEST(Parse, MissingSuffix) {
    auto res = parse_target("/{a}/{b}", "/aa");
    ASSERT_FALSE(res);
}
