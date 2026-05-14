#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <plai/ctx/playlist_cursor.hpp>

using Cursor = plai::ctx::PlaylistCursor;
using testing::ElementsAre;

auto next_n(Cursor& c, size_t count) {
    std::vector<std::string> out{};
    while (count--) { out.push_back(c.next()); }
    return out;
}

template <class... Ts>
auto mk_strvec(Ts&&... ts) {
    // NOLINTNEXTLINE
    return std::vector<std::string>{std::forward<Ts>(ts)...};
}

TEST(Ctor, Default) {
    auto c = Cursor();
    ASSERT_TRUE(c.empty());
}

TEST(Ctor, Literals) {
    auto c = Cursor({"foo", "bar", "baz"});
    ASSERT_EQ(c.entries().size(), 3);
}

TEST(Next, Empty) {
    auto c = Cursor();
    auto v = c.next();
    ASSERT_EQ(v, "");
    v = c.next();
    ASSERT_EQ(v, "");
}

TEST(Next, Values) {
    auto c = Cursor{"foo", "bar"};
    auto vals = next_n(c, 4);
    ASSERT_THAT(vals, ElementsAre("foo", "bar", "", "foo"));
}

TEST(Trim, One) {
    auto c = Cursor{"foo", "bar"};
    c.trim_to_size(1);
    auto vals = next_n(c, 2);
    ASSERT_THAT(vals, ElementsAre("bar", ""));
}

TEST(Insert, Front) {
    auto c = Cursor{"foo", "bar"};
    (void)c.next();
    auto v = mk_strvec("baz", "asd");
    c.insert_front(v);
    auto vals = next_n(c, 4);
    ASSERT_THAT(vals, ElementsAre("bar", "", "baz", "asd"));
}

TEST(Insert, Back) {
    auto c = Cursor{"foo", "bar"};
    (void)c.next();
    auto v = mk_strvec("baz", "asd");
    c.insert_back(v);
    auto vals = next_n(c, 4);
    ASSERT_THAT(vals, ElementsAre("bar", "baz", "asd", ""));
}

TEST(Insert, Next) {
    auto c = Cursor{"foo", "bar"};
    (void)c.next();
    auto v = mk_strvec("baz", "asd");
    c.insert_next(v);
    auto vals = next_n(c, 4);
    ASSERT_THAT(vals, ElementsAre("baz", "asd", "bar", ""));
}
