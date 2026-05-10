#include <plai/ctx/multi_playlist_cursor.hpp>

namespace plai::ctx {
namespace {
constexpr auto npos = std::numeric_limits<size_t>::max();
auto find_by_name(auto& collection, std::string_view nm) {
    return std::ranges::find_if(collection,
                                [&](const auto& i) { return i->name == nm; });
}
}  // namespace

auto MultiPlaylistCursor::operator[](std::string_view nm) -> Playlist& {
    auto it = find_by_name(m_playlists, nm);
    if (it == m_playlists.end()) {
        m_playlists.emplace_back();
        return *m_playlists.back();
    }
    return **it;
}
bool MultiPlaylistCursor::erase(std::string_view nm) {
    auto it = find_by_name(m_playlists, nm);
    if (it == m_playlists.end()) return false;
    const auto delta = static_cast<size_t>(it - m_playlists.begin());
    if (!m_tmp && delta == m_idx) {
        // Deleting the currently active playlist
        // If tmp were true the current playlist would have been cached in m_tmp
        m_tmp = std::exchange(m_playlists[m_idx], {});
    } else if (delta < m_idx) {
        // Keep the active index correct
        --m_idx;
    }
    m_playlists.erase(it);
    return true;
}

auto MultiPlaylistCursor::next() noexcept -> Playlist* {
    if (m_tmp) m_tmp.reset();
    while (m_idx < m_playlists.size() && !m_playlists[m_idx]->active) ++m_idx;
    if (m_idx >= m_playlists.size()) {
        m_idx = npos;
        return nullptr;
    }
    return m_playlists[m_idx++].get();
}

void MultiPlaylistCursor::reset() noexcept { m_idx = 0; }

}  // namespace plai::ctx
