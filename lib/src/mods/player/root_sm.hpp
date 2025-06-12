#pragma once

#include <plai/st/state_machine.hpp>

#include "mods/player/blend_sm.hpp"
#include "mods/player/ctx.hpp"
#include "mods/player/image_sm.hpp"

namespace plai::mods::player {

enum class RootSt {
    Init,
    Vid,
    Img,
    Vid2Vid,
    Vid2Img,
    Img2Vid,
    Img2Img,
    Done,
};

class RootSm {
    using enum RootSt;

 public:
    using state_type = RootSt;

    explicit RootSm(Ctx& ctx) noexcept : m_ctx(&ctx) {}

    RootSm(const RootSm&) = delete;
    RootSm& operator=(const RootSm&) = default;
    RootSm(RootSm&&) = delete;
    RootSm& operator=(RootSm&&) = delete;

    ~RootSm() = default;

    state_type step(st::tag_t<Init>);
    state_type step(st::tag_t<Vid>);
    state_type step(st::tag_t<Img>);
    state_type step(st::tag_t<Vid2Vid>);
    state_type step(st::tag_t<Vid2Img>);
    state_type step(st::tag_t<Img2Vid>);
    state_type step(st::tag_t<Img2Img>);

 private:
    Ctx* m_ctx;
    st::StateMachine<ImageSm> m_img_sm{std::in_place, *m_ctx};
    st::StateMachine<BlendSm> m_blend_sm{std::in_place, *m_ctx};
};
}  // namespace plai::mods::player
