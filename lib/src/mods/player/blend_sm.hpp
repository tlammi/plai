#pragma once

#include <plai/st/tag.hpp>
#include <plai/time.hpp>

#include "mods/alpha_calc.hpp"
#include "mods/player/ctx.hpp"

namespace plai::mods::player {

enum class BlendSt {
    Init,
    WatermarkFadeIn,
    Blend,
    WatermarkFadeOut,
    Done,
};
class BlendSm {
 public:
    using enum BlendSt;
    using state_type = BlendSt;
    BlendSm(Ctx& ctx) : m_ctx(&ctx) {}

    void setup(bool src_img, bool dst_img) {
        m_src_img = src_img;
        m_dst_img = dst_img;
    }

    state_type step(st::tag_t<Init>);
    state_type step(st::tag_t<WatermarkFadeIn>);
    state_type step(st::tag_t<Blend>);
    state_type step(st::tag_t<WatermarkFadeOut>);

    void reset() {}

 private:
    Ctx* m_ctx{};
    TimePoint m_tstamp{};
    AlphaCalc m_alpha{};
    bool m_src_img{};
    bool m_dst_img{};
};  // namespace plai::mods::player
}  // namespace plai::mods::player
