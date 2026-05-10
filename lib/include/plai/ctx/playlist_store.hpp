#pragma once

#include <plai/c_str.hpp>
#include <plai/time.hpp>
#include <plai/virtual.hpp>
#include <variant>

namespace plai::ctx {

class PlaylistStore : public Virtual {
 public:
    struct SetArgs {
        std::optional<std::span<const std::string>> medias{};
        std::optional<bool> active{};
        std::optional<Duration> image_duration{};
    };
    /**
     * \brief Set the playlist specification
     * */
    virtual void set(CStr nm, SetArgs args) = 0;

    virtual bool contains(CStr nm) = 0;

    /**
     * \brief Remove a playlist
     * */
    virtual bool remove(CStr nm) = 0;

    enum class Location {
        Front,  ///< Insert to front of the list
        Back,   ///< Insert to back of the list

        /// Insert after the current "cursor position" if the current
        /// playlist is being played or to the front of the list if not
        Next,
    };

    /**
     * \brief Add new media to the playlist
     *
     * \param nm Playlist name
     * \param loc Insertion location
     * \param medias Medias to insert
     * */
    virtual void amend(CStr nm, Location loc,
                       std::span<const std::string> medias) = 0;

    /**
     * \brief Trim the playlist
     *
     * Removes the oldest items from the playlist until it reaches the given
     * size. If the playlist has the same size already or is smaller nothing is
     * done.
     *
     * \return Number of items removed
     * */
    virtual size_t trim(CStr nm, size_t size) = 0;

    struct PlaylistEvt {
        std::string name;
        Duration image_duration;
    };
    struct MediaEvt {
        std::string name;
    };
    struct NullEvt {};

    using Evt = std::variant<PlaylistEvt, MediaEvt, NullEvt>;

    virtual Evt next() = 0;
};

std::unique_ptr<PlaylistStore> simple_playlist_store();

}  // namespace plai::ctx
