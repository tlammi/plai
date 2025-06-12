#pragma once

#include <mutex>
#include <plai/frontend/texture.hpp>
#include <plai/media/frame.hpp>
#include <plai/media/frame_converter.hpp>
#include <plai/mods/decoder.hpp>
#include <plai/play/player.hpp>
#include <plai/sched/task.hpp>
#include <plai/vec.hpp>

namespace plai::mods::player {

constexpr auto MAIN_TARGET =
    RenderTarget{.vertical = Position::Middle, .horizontal = Position::Middle};

constexpr auto WATERMARK_BLEND = std::chrono::milliseconds(500);
constexpr Duration PERIOD_30MS = std::chrono::microseconds(33'333);
constexpr Duration PERIOD_60MS = std::chrono::microseconds(16'667);
constexpr auto MAX_ALPHA = std::numeric_limits<uint8_t>::max();

struct Ctx {
    static constexpr auto DEFAULT_DIMS = Vec<int>{1920, 1080};
    std::mutex mut{};
    std::optional<Decoded> buf{};
    play::PlayerOpts opts{};
    media::Frame frm{};
    media::Frame prev_frm{};
    Vec<int> dims{DEFAULT_DIMS};
    media::FrameConverter m_conv{};

    std::unique_ptr<Texture> text;
    std::unique_ptr<Texture> back;
    std::vector<std::unique_ptr<Texture>> watermark_textures{};
    sched::PeriodicTask* task{};

    std::function<void()> notify_sink_ready;

    std::optional<Decoded> extract_buf();

    void render_watermarks(uint8_t alpha = MAX_ALPHA);
};
}  // namespace plai::mods::player
