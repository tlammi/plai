#pragma once

#include <plai/st/tag.hpp>

#include "mods/player/ctx.hpp"

namespace plai::mods::player {

enum class ImageSt {
    Init,
    Show,
    Delay,
    End,
    Done,
};

class ImageSm {
 public:
    using enum ImageSt;
    using state_type = ImageSt;
    explicit ImageSm(Ctx& ctx) : m_ctx(&ctx) {}

    state_type step(st::tag_t<Init>);
    state_type step(st::tag_t<Show>);
    state_type step(st::tag_t<Delay>);
    state_type step(st::tag_t<End>);

    constexpr void reset() const noexcept {}

 private:
    Ctx* m_ctx;
    TimePoint m_tstamp{};
};

}  // namespace plai::mods::player
