#pragma once

#include <plai/ctx/playlist_cursor.hpp>
#include <plai/time.hpp>
#include <vector>

namespace plai::ctx {

class MultiPlaylistCursor {
 public:
    struct Playlist {
        std::string name;
        PlaylistCursor cursor{};
        Duration image_duration{};
        bool active{false};
    };

    Playlist& operator[](std::string_view nm);

    bool erase(std::string_view nm);

    Playlist* next() noexcept;

    void reset() noexcept;

 private:
    std::vector<std::unique_ptr<Playlist>> m_playlists{};

    std::unique_ptr<Playlist> m_tmp{};  // Stores the playlist if the currently
                                        // active playlist was removed
    size_t m_idx{};
};
}  // namespace plai::ctx
