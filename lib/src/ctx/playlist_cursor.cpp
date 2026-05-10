#include <algorithm>
#include <cassert>
#include <limits>
#include <plai/ctx/playlist_cursor.hpp>
#include <ranges>
namespace plai::ctx {

namespace rv = std::views;

namespace {

static constexpr auto npos = std::numeric_limits<size_t>::max();

constexpr auto to_elem(auto& id_store) {
    return rv::transform([&](const auto& str) {
        return PlaylistCursor::Entry{
            .name = str,
            .id = id_store++,
        };
    });
}
}  // namespace

void PlaylistCursor::reset() noexcept { m_idx = 0; }

void PlaylistCursor::overwrite(std::span<const std::string> data) {
    m_medias = data | to_elem(m_id) | std::ranges::to<std::vector>();
    reset();
}

void PlaylistCursor::insert_front(std::span<const std::string> data) {
    m_medias.insert_range(m_medias.begin(), data | to_elem(m_id));
    // Only move the index if the cursor has already consumed items
    if (m_idx > 0) m_idx += data.size();
}

void PlaylistCursor::insert_back(std::span<const std::string> data) {
    m_medias.append_range(data | to_elem(m_id));
}

void PlaylistCursor::insert_next(std::span<const std::string> data) {
    if (m_idx >= m_medias.size()) {
        insert_front(data);
        return;
    }
    auto it = m_medias.begin() +
              static_cast<decltype(m_medias)::difference_type>(m_idx);
    m_medias.insert_range(it, data | to_elem(m_id));
#ifndef NDEBUG
    auto subspan = std::span(m_medias).subspan(m_idx, data.size());
    for (const auto& [entry, name] : rv::zip(subspan, data)) {
        assert(entry.name == name);
    }
#endif
}

size_t PlaylistCursor::trim_to_size(size_t max_size) {
    const auto orig_size = m_medias.size();
    if (max_size > m_medias.size()) return 0;
    if (max_size == 0) {
        m_medias.clear();
        return orig_size;
    }
    auto ids = m_medias |
               rv::transform([](const auto& item) { return item.id; }) |
               std::ranges::to<std::vector>();
    std::ranges::sort(ids);
    auto cutoff = ids[ids.size() - max_size];
    // Use the current ID to check if we need to decrement the current index to
    // keep the same ative media.
    const auto active_id = m_idx < m_medias.size() ? m_medias[m_idx].id : npos;
    m_medias = std::move(m_medias) |
               rv::filter([&](const auto& item) { return item.id >= cutoff; }) |
               std::ranges::to<std::vector>();
    assert(m_medias.size() == max_size);

    // Try to restore the index to point to the same object
    auto it = std::ranges::find_if(
        m_medias, [&](const auto& pair) { return pair.id == active_id; });
    if (it != m_medias.end())
        m_idx = it - m_medias.begin();
    else
        m_idx = 0;
    return orig_size - m_medias.size();
}

std::string PlaylistCursor::next() {
    if (m_idx >= m_medias.size()) return {};
    return m_medias[m_idx++].name;
}

}  // namespace plai::ctx
