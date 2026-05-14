#pragma once

#include <cstddef>
#include <plai/exceptions.hpp>
#include <plai/virtual.hpp>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace plai::ctx {

/**
 * \brief Type for managing a playlist state when playing it
 *
 * This type handles things like adding new medias to the playlist while it is
 * being played or removal of the medias with as little effect on the playing as
 * possible.
 * */
class PlaylistCursor {
 public:
    struct Entry {
        std::string name;
        size_t id;
    };

    constexpr PlaylistCursor() noexcept = default;

    explicit PlaylistCursor(std::vector<Entry> entries) noexcept
        : m_medias(std::move(entries)) {}

    template <std::ranges::range R>
    explicit PlaylistCursor(R&& r)
        requires(!std::same_as<std::remove_cvref_t<R>, std::vector<Entry>>)
        : m_medias(std::forward<R>(r) | std::views::transform([&](auto&& v) {
                       return Entry{
                           .name = std::forward<decltype(v)>(v),
                           .id = 0,
                       };
                   }) |
                   std::ranges::to<std::vector>()) {
        for (auto& m : m_medias) m.id = m_id++;
    }

    template <class T>
    explicit PlaylistCursor(std::initializer_list<T> list)
        : PlaylistCursor(std::views::all(list)) {}

    PlaylistCursor(const PlaylistCursor&) = default;
    PlaylistCursor& operator=(const PlaylistCursor&) = default;

    PlaylistCursor(PlaylistCursor&&) noexcept = default;
    PlaylistCursor& operator=(PlaylistCursor&&) noexcept = default;

    ~PlaylistCursor() = default;

    constexpr bool empty() const noexcept { return m_medias.empty(); }
    /**
     * \brief Access data for serialization
     * */
    const auto& entries() const noexcept { return m_medias; }

    /**
     * \brief Set the medias
     *
     * This overwrites the medias gracefully moving from an actively played
     * playlist to the new one.
     * */
    void overwrite(std::span<const std::string> data);

    /**
     * \brief Insert new medias to the front of the list
     * */
    void insert_front(std::span<const std::string> data);
    /**
     * \brief Insert new medias to the end of the list
     * */
    void insert_back(std::span<const std::string> data);

    /**
     * \brief Insert new medias to current cursor location
     *
     * This inserts entries so they are returned by the cursor next.
     * */
    void insert_next(std::span<const std::string> data);

    /**
     * \brief
     * */
    size_t trim_to_size(size_t max_size);

    /**
     * \brief Read next media
     *
     * \return Empty string on end, otherwise next media name
     * */
    [[nodiscard]] std::string next();

 private:
    std::vector<Entry> m_medias{};
    // Current media index
    //
    size_t m_idx{0};

    // Id for the next media
    size_t m_id{};
};
}  // namespace plai::ctx
