#include <algorithm>
#include <cassert>
#include <plai/ctx/playlist_store.hpp>
#include <plai/exceptions.hpp>
#include <ranges>

namespace plai::ctx {

namespace {
constexpr auto DEFAULT_IMG_DURATION = std::chrono::seconds(5);
constexpr auto npos = std::numeric_limits<size_t>::max();

struct MediaPair {
    std::string name;
    size_t id;
};

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
        if (idx == orig_idx && !playlists[idx].active) idx = npos;
    }
    assert(idx < playlists.size() || idx == npos);
    return idx;
}

constexpr auto find_by_name(auto& container, CStr nm) noexcept {
    return std::ranges::find_if(container,
                                [&](const auto& i) { return i.name == nm; });
}

auto mk_media_vec(std::span<const std::string> data, auto& id) {
    return data | std::views::transform([&](const auto& str) {
               return MediaPair{.name = str, .id = {id++}};
           }) |
           std::ranges::to<std::vector>();
}

auto amend_media_vec_front(std::vector<MediaPair>&& medias, auto& id,
                           std::span<const std::string> new_medias) {
    // TODO: Could use std::views::concat with a newer compiler
    auto out = std::remove_cvref_t<decltype(medias)>();
    out.reserve(medias.size() + new_medias.size());
    out.append_range(new_medias | std::views::transform([&](const auto& nm) {
                         return MediaPair{.name = nm, .id = id++};
                     }));
    out.append_range(std::move(medias));
    return out;
}

auto amend_media_vec_back(std::vector<MediaPair>&& medias, auto& id,
                          std::span<const std::string> new_medias) {
    medias.append_range(new_medias | std::views::transform([&](const auto& nm) {
                            return MediaPair{.name = nm, .id = id++};
                        }));
    return std::move(medias);
}

auto amend_media_vec_before(std::vector<MediaPair>&& medias, auto& id,
                            std::span<const std::string> new_medias, auto idx) {
    assert(idx <= medias.size());
    if (idx == 0)
        return amend_media_vec_front(std::move(medias), id, new_medias);
    if (idx == medias.size())
        return amend_media_vec_back(std::move(medias), id, new_medias);
    auto out = std::remove_cvref_t<decltype(medias)>();
    out.reserve(medias.size() + new_medias.size());
    auto front_span = std::span(medias).subspan(0, idx);
    for (auto m : front_span) out.push_back(std::move(m));
    out.append_range(new_medias | std::views::transform([&](const auto& nm) {
                         return MediaPair{.name = nm, .id = id++};
                     }));
    auto back_span = std::span(medias).subspan(idx);
    for (auto m : back_span) out.push_back(std::move(m));
    return out;
}

void trim_by_removing_oldest(std::vector<MediaPair>& medias, size_t new_size) {
    assert(new_size < medias.size());
    if (new_size == 0) {
        medias.clear();
        return;
    }
    auto ids = medias |
               std::views::transform([](const auto& pair) { return pair.id; }) |
               std::ranges::to<std::vector>();
    assert(ids.size() == medias.size());
    std::ranges::sort(ids);  // media ids with oldes at the front
    const auto oldest_id_to_keep = ids[ids.size() - new_size];
    medias = std::move(medias) | std::views::filter([&](const auto& pair) {
                 return pair.id >= oldest_id_to_keep;
             }) |
             std::ranges::to<std::vector>();
    assert(medias.size() == new_size);
}

}  // namespace

class SimplePlaylistStore final : public PlaylistStore {
 public:
    void set(CStr nm, SetArgs args) override {
        auto it = find_by_name(m_playlists, nm);
        if (it == m_playlists.end()) {
            auto medias = args.medias ? mk_media_vec(*args.medias, m_media_id)
                                      : std::vector<MediaPair>();
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
            it->medias = mk_media_vec(*args.medias, m_media_id);
        }
        if (args.image_duration) it->image_duration = *args.image_duration;
        if (args.active) it->active = *args.active;
    }
    bool contains(CStr nm) override {
        auto it = find_by_name(m_playlists, nm);
        return it != m_playlists.end();
    }

    bool remove(CStr nm) override {
        auto it = find_by_name(m_playlists, nm);
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

    void amend(CStr nm, Location loc,
               std::span<const std::string> medias) override {
        auto it = find_by_name(m_playlists, nm);
        if (it == m_playlists.end())
            throw KeyError("SimplePlaylistStore::amend()");
        const auto delta = static_cast<size_t>(it - m_playlists.begin());
        using enum Location;
        if (delta == m_pl_idx) {
            switch (loc) {
                case Front:
                    it->medias = amend_media_vec_front(std::move(it->medias),
                                                       m_media_id, medias);
                    m_media_idx += medias.size();
                    return;
                case Back:
                    it->medias = amend_media_vec_back(std::move(it->medias),
                                                      m_media_id, medias);
                    return;
                case Next:
                    it->medias = amend_media_vec_before(std::move(it->medias),
                                                        m_media_id, medias,
                                                        m_media_idx + 1);
                    return;
            }
        }
        switch (loc) {
            case Next: [[fallthrough]];
            case Front:
                it->medias = amend_media_vec_front(std::move(it->medias),
                                                   m_media_id, medias);
                return;
            case Back:
                it->medias = amend_media_vec_back(std::move(it->medias),
                                                  m_media_id, medias);
                return;
        }
    }
    size_t trim(CStr nm, size_t size) override {
        auto it = find_by_name(m_playlists, nm);
        if (it == m_playlists.end())
            throw KeyError("SimplePlaylistStore::trim()");
        if (it->medias.size() <= size) return 0;
        const auto delta = static_cast<size_t>(it - m_playlists.begin());
    }

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
                .name = m_tmp.medias[m_media_idx].name,
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
            .name = m_playlists[m_pl_idx].medias[m_media_idx].name,
        };
        ++m_media_idx;
        if (m_media_idx == m_playlists[m_pl_idx].medias.size())
            m_media_idx = npos;
        return res;
    }

 private:
    struct PlaylistInfo {
        std::string name;
        std::vector<MediaPair> medias;
        Duration image_duration;
        bool active;
    };

    std::vector<PlaylistInfo> m_playlists{};
    // Used as a working copy when the currently played playlist is modified.
    PlaylistInfo m_tmp{};
    size_t m_pl_idx{npos};
    size_t m_media_idx{npos};
    // Used for tracking the media insertion order
    size_t m_media_id{};
};
std::unique_ptr<PlaylistStore> simple_playlist_store() {
    return std::make_unique<SimplePlaylistStore>();
}
}  // namespace plai::ctx
