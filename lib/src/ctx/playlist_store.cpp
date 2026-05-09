#include <cassert>
#include <plai/ctx/playlist_store.hpp>

namespace plai::ctx {

namespace {
constexpr auto DEFAULT_IMG_DURATION = std::chrono::seconds(5);
constexpr auto npos = std::numeric_limits<size_t>::max();

// Search for the next active playlist in the list
//
// Returns npos if no active playlists exist
constexpr size_t next_active_playlist(const auto& playlists,
                                      const size_t orig_idx) noexcept {
    assert(orig_idx < playlists.size() && "playlist idx out of bounds");
    auto idx = orig_idx + 1;
    while (idx < playlists.size() && !playlists[idx].active) ++idx;
    if (idx == playlists.size()) {
        idx = 0;
        while (idx < orig_idx && !playlists[idx].active) ++idx;
        if (idx == orig_idx) idx = npos;
    }
    assert(idx < playlists.size() || idx == npos);
    return idx;
}
}  // namespace

class SimplePlaylistStore final : public PlaylistStore {
 public:
    void set(CStr nm, SetArgs args) override {
        auto it = std::ranges::find_if(
            m_playlists, [&](const auto& i) { return i.name == nm; });
        if (it == m_playlists.end()) {
            auto medias = args.medias
                              ? std::vector<std::string>(args.medias->begin(),
                                                         args.medias->end())
                              : std::vector<std::string>{};
            m_playlists.push_back({
                .name = std::string(nm),
                .medias = std::move(medias),
                .image_duration =
                    args.image_duration.value_or(DEFAULT_IMG_DURATION),
                .active = args.active.value_or(false),
            });
            return;
        }
        const auto delta = static_cast<size_t>(it - m_playlists.begin());

        if (args.medias) {
            if (delta == m_pl_idx) {
                // modifying the currently played playlist -> making a copy
                // TODO: the medias could be moved here since those are
                // overwritten next
                m_tmp = m_playlists[m_pl_idx];
                m_pl_idx = next_active_playlist(m_playlists, m_pl_idx);
            }
            it->medias = std::vector(args.medias->begin(), args.medias->end());
        }
        if (args.image_duration) it->image_duration = *args.image_duration;
        if (args.active) it->active = *args.active;
    }
    bool contains(CStr nm) override {
        auto it = std::ranges::find_if(
            m_playlists, [&](const auto& p) { return p.name == nm; });
        return it != m_playlists.end();
    }

    bool remove(CStr nm) override {
        auto it = std::ranges::find_if(
            m_playlists, [&](const auto& i) { return i.name == nm; });
        if (it == m_playlists.end()) return false;
        const auto delta = static_cast<size_t>(it - m_playlists.begin());
        if (delta == m_pl_idx) {
            // Removing the currently played
            m_tmp = m_playlists[m_pl_idx];
            m_playlists.erase(it);
            m_pl_idx = next_active_playlist(m_playlists, m_pl_idx);
            return true;
        }
        m_playlists.erase(it);
        if (delta < m_pl_idx) {
            // Erased element before current playlist -> index changed
            --m_pl_idx;
        }
        return true;
    }

    void rotate(CStr nm, Location loc,
                std::span<const std::string> medias) override {}

    Evt next() override {
        if (!m_tmp.name.empty()) {
            // changes were done to the current playlist so use temporary copy
            // until that is processed.
            if (m_media_idx == npos) {
                m_tmp.name.clear();
                // playlist end has been reached
                m_media_idx = 0;
                return PlaylistEvt{
                    .name = m_playlists[m_pl_idx].name,
                    .image_duration = m_playlists[m_pl_idx].image_duration,
                };
            }
            auto evt = MediaEvt{
                .name = m_tmp.medias[m_media_idx],
            };
            ++m_media_idx;
            if (m_media_idx == m_tmp.medias.size()) m_media_idx = npos;
            return evt;
        }
        if (m_pl_idx == npos) {
            if (m_playlists.empty()) return NullEvt{};
            m_pl_idx = 0;
            if (!m_playlists[m_pl_idx].active)
                m_pl_idx = next_active_playlist(m_playlists, m_pl_idx);
            if (m_pl_idx == npos) return NullEvt{};
            m_media_idx = 0;
            return PlaylistEvt{
                .name = m_playlists[m_pl_idx].name,
                .image_duration = m_playlists[m_pl_idx].image_duration,
            };
        }
        if (m_media_idx == npos) {
            m_pl_idx = next_active_playlist(m_playlists, m_pl_idx);
            if (m_pl_idx == npos) return NullEvt{};
            m_media_idx = 0;
            return PlaylistEvt{
                .name = m_playlists[m_pl_idx].name,
                .image_duration = m_playlists[m_pl_idx].image_duration,
            };
        }
        auto res = MediaEvt{
            .name = m_playlists[m_pl_idx].medias[m_media_idx],
        };
        ++m_media_idx;
        if (m_media_idx == m_playlists[m_pl_idx].medias.size())
            m_media_idx = npos;
        return res;
    }

 private:
    struct PlaylistInfo {
        std::string name;
        std::vector<std::string> medias;
        Duration image_duration;
        bool active;
    };

    std::vector<PlaylistInfo> m_playlists{};
    // Used as a working copy when the currently played playlist is modified.
    PlaylistInfo m_tmp{};
    size_t m_pl_idx{npos};
    size_t m_media_idx{npos};
};
std::unique_ptr<PlaylistStore> simple_playlist_store() {
    return std::make_unique<SimplePlaylistStore>();
}
}  // namespace plai::ctx
