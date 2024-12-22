#pragma once

#include <memory>
#include <plai/frontend/frontend.hpp>
#include <plai/play/media_src.hpp>
#include <plai/time.hpp>

namespace plai::play {

/**
 * \brief Image to render on top of each real frame
 *
 * This can be used to render e.g. footer image to the bottom
 * of the screen. This will be overlaid on top each frame.
 * */
struct Watermark {
    media::Frame image{};
    RenderTarget target{};
};

static constexpr auto IMAGE_DEFAULT_DURATION = std::chrono::seconds(5);

struct PlayerOpts {
    /// Watermarks. Rendered in order
    std::vector<Watermark> watermarks{};

    /**
     * \brief How long to display images
     * */
    Duration image_dur{IMAGE_DEFAULT_DURATION};

    /**
     * \brief Whether to wait if there is no more media available.
     *
     * If this is set to false the player will block in case there is no media
     * available. If set to true the player exists once the media provider
     * returns std::nullopt.
     * */
    bool wait_media{false};

    /**
     * \brief If set to true the video framerate is not limited
     *
     * Enabling this can be used to test the theoretical rendering speed
     * to check the hardware capabilities.
     * */
    bool unlimited_fps{false};
};

class Player {
 public:
    /**
     * \brief Create a player
     *
     * \param front Frontend to render the medias
     * \param media_src Media provider
     * \param opts Player options
     * */
    Player(Frontend* front, MediaSrc* media_src, PlayerOpts opts = {});

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    Player(Player&&) noexcept = default;
    Player& operator=(Player&&) noexcept = default;

    ~Player();

    /**
     * \brief Run the player
     * */
    void run();

 private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
}  // namespace plai::play
