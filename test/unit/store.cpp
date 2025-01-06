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

TEST(Lock, MultiMatch) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    db->store("b", span_cast("b"));
    db->store("c", span_cast("c"));
    auto keys = std::vector<plai::CStr>{"a", "b"};
    auto success = db->lock(keys);
    ASSERT_TRUE(success);
    auto res = db->inspect("a");
    ASSERT_TRUE(res.value().locked);
    res = db->inspect("b");
    ASSERT_TRUE(res.value().locked);
    res = db->inspect("c");
    ASSERT_FALSE(res.value().locked);
}

TEST(Lock, SingleMismatch) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    auto keys = std::vector<plai::CStr>{"b"};
    auto success = db->lock(keys);
    ASSERT_FALSE(success);
    auto res = db->inspect("a");
    ASSERT_FALSE(res.value().locked);
}

TEST(Lock, MultiMisMatch) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    db->store("b", span_cast("b"));
    db->store("c", span_cast("c"));

    auto keys = std::vector<plai::CStr>{"a", "b", "c", "d"};
    auto success = db->lock(keys);
    ASSERT_FALSE(success);
    keys.pop_back();
    for (const auto k : keys) {
        auto res = db->inspect(k);
        ASSERT_FALSE(res.value().locked);
    }
}

TEST(Unlock, One) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    auto keys = std::vector<plai::CStr>{"a"};
    bool success = db->lock(keys);
    ASSERT_TRUE(success);
    db->unlock(keys);
    auto res = db->inspect("a");
    ASSERT_FALSE(res.value().locked);
}

TEST(Unlock, Many) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    db->store("b", span_cast("b"));
    db->store("c", span_cast("c"));
    auto keys = std::vector<plai::CStr>{"a", "b", "c"};
    auto success = db->lock(keys);
    ASSERT_TRUE(success);
    keys.pop_back();
    db->unlock(keys);

    ASSERT_FALSE(db->inspect("a").value().locked);
    ASSERT_FALSE(db->inspect("b").value().locked);
    ASSERT_TRUE(db->inspect("c").value().locked);
}

TEST(Remove, One) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    db->remove("a");
    auto ls = db->list();
    ASSERT_TRUE(ls.empty());
}

TEST(Remove, OneLocked) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    auto keys = std::vector<plai::CStr>{"a"};
    db->lock(keys);
    db->remove("a");
    auto res = db->inspect("a");
    ASSERT_TRUE(res);
    ASSERT_TRUE(res->marked_for_deletion);
}

TEST(Remove, UnlockMarked) {
    auto db = mk_store();
    db->store("a", span_cast("a"));
    auto keys = std::vector<plai::CStr>{"a"};
    db->lock(keys);
    db->remove("a");
    db->unlock(keys);
    auto ls = db->list();
    ASSERT_TRUE(ls.empty());
}

TEST(Read, Simple) {
    auto db = mk_store();
    auto span = span_cast("a");
    auto expected = std::vector<uint8_t>(span.begin(), span.end());
    db->store("a", expected);
    auto res = db->read("a");
    ASSERT_EQ(res, expected);
}
