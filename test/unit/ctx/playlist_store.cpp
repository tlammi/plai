#include <gtest/gtest.h>

#include <plai/ctx/playlist_store.hpp>

using plai::ctx::PlaylistStore;
using Evt = PlaylistStore::Evt;
using PlaylistEvt = PlaylistStore::PlaylistEvt;
using MediaEvt = PlaylistStore::MediaEvt;
using NullEvt = PlaylistStore::NullEvt;

using StoreFn = std::function<std::unique_ptr<PlaylistStore>()>;

class Test : public ::testing::TestWithParam<StoreFn> {
 public:
    std::unique_ptr<PlaylistStore> str{};

    void SetUp() override { str = GetParam()(); }
};

INSTANTIATE_TEST_SUITE_P(AllImplementations, Test, ::testing::Values([] {
                             return plai::ctx::simple_playlist_store();
                         }));

TEST_P(Test, CreatePlaylist) {
    str->set("foo", {});
    ASSERT_TRUE(str->contains("foo"));
    ASSERT_FALSE(str->contains("bar"));
}

TEST_P(Test, Remove) {
    str->set("foo", {});
    ASSERT_TRUE(str->remove("foo"));
    ASSERT_FALSE(str->remove("foo"));
}

TEST_P(Test, EvtEmpty) {
    auto res = str->next();
    ASSERT_TRUE(std::holds_alternative<PlaylistStore::NullEvt>(res));
}

template <class... Ts>
constexpr auto mk_strvec(Ts&&... ts) {
    return std::vector<std::string>{std::forward<Ts>(ts)...};
}

TEST_P(Test, EvtNoActive) {
    auto m = mk_strvec("m1", "m2");
    str->set("foo", {.medias = m});
    auto res = str->next();
    ASSERT_TRUE(std::holds_alternative<PlaylistStore::NullEvt>(res));
}

TEST_P(Test, EvtActive) {
    auto m = mk_strvec("m1", "m2");
    str->set("foo", {.medias = m, .active = true});
    auto evt = str->next();
    auto* plist = std::get_if<PlaylistEvt>(&evt);
    ASSERT_TRUE(plist);
    ASSERT_EQ(plist->name, "foo");
    evt = str->next();
    auto* media = std::get_if<MediaEvt>(&evt);
    ASSERT_TRUE(media);
    ASSERT_EQ(media->name, "m1");
    evt = str->next();
    media = std::get_if<MediaEvt>(&evt);
    ASSERT_TRUE(media);
    ASSERT_EQ(media->name, "m2");
    evt = str->next();
    plist = std::get_if<PlaylistEvt>(&evt);
}
