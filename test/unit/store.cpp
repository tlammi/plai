#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <plai/crypto.hpp>
#include <plai/store.hpp>

using testing::ElementsAre;
using testing::UnorderedElementsAre;

auto mk_store() { return plai::sqlite_store(":memory:"); }

std::span<const uint8_t> span_cast(std::span<const char> spn) {
    return {reinterpret_cast<const uint8_t*>(spn.data()), spn.size()};
}

TEST(Init, Create) { mk_store(); }

TEST(List, Empty) {
    auto db = mk_store();
    ASSERT_TRUE(db->list().empty());
}

TEST(Store, One) {
    auto db = mk_store();
    db->store("foo", span_cast("bar"));
    auto res = db->list();
    ASSERT_THAT(res, ElementsAre("foo"));
}

TEST(Store, Multiple) {
    auto db = mk_store();
    db->store("foo", span_cast("bar"));
    db->store("bar", span_cast("baz"));
    db->store("baz", span_cast("asd"));
    auto res = db->list();
    ASSERT_THAT(res, UnorderedElementsAre("foo", "bar", "baz"));
}

TEST(Inspect, Miss) {
    auto db = mk_store();
    auto res = db->inspect("foo");
    ASSERT_FALSE(res);
}

TEST(Inspect, Match) {
    auto db = mk_store();
    db->store("foo", span_cast("bar"));
    auto res = db->inspect("foo");
    ASSERT_TRUE(res);
    ASSERT_EQ(res->bytes, 4);
    const auto expected = plai::crypto::sha256(span_cast("bar"));
    ASSERT_EQ(res->sha256, expected);
}

TEST(Store, Overwrite) {
    auto db = mk_store();
    db->store("foo", span_cast("bar"));
    db->store("foo", span_cast("baz"));
    auto elements = db->list();
    ASSERT_THAT(elements, ElementsAre("foo"));
    auto res = db->inspect("foo");
    ASSERT_TRUE(res);
    ASSERT_EQ(res->bytes, 4);
    ASSERT_EQ(res->sha256, plai::crypto::sha256(span_cast("baz")));
}

TEST(Store, OverwriteSome) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    db->store("b", span_cast("b"));
    db->store("c", span_cast("c"));
    db->store("b", span_cast("foo"));
    auto keys = db->list();
    ASSERT_THAT(keys, UnorderedElementsAre("a", "b", "c"));

    auto res = db->inspect("b");
    ASSERT_TRUE(res);
    ASSERT_EQ(res->bytes, 4);
    ASSERT_EQ(res->sha256, plai::crypto::sha256(span_cast("foo")));
}

TEST(Lock, OneMatch) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    auto res = db->inspect("a");
    ASSERT_TRUE(res);
    ASSERT_FALSE(res->locked);
    auto keys = std::vector<plai::CStr>{"a"};
    auto success = db->lock(keys);
    ASSERT_TRUE(success);
    res = db->inspect("a");
    ASSERT_TRUE(res);
    ASSERT_TRUE(res->locked);
}

