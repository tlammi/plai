#pragma once

#include <optional>
#include <plai/media/media.hpp>
#include <plai/virtual.hpp>

namespace plai::play {

class MediaSrc : public Virtual {
 public:
    /**
     * \brief Get the next media
     *
     * Used by player to get the next media to play. Returning
     * std::nullopt indicates that no media is available. Depending
     * on the player settings this might cause the player to return
     * after all media have been played or to block until more data is
     * available.
     * */
    virtual std::optional<media::Media> next_media() = 0;
};
}  // namespace plai::play
