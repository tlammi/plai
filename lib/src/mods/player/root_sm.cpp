#include "root_sm.hpp"

#include <plai/mods/player.hpp>
#include <plai/util/match.hpp>

namespace plai::mods::player {
using namespace std::literals;
namespace {
constexpr Duration meta_to_period(const DecodingMeta& meta) {
    if (meta.still()) return 100ms;
    static constexpr auto micro = 1'000'000;
    auto uspf = meta.fps.reciprocal() * micro;
    auto uspf_int =
        static_cast<uint64_t>(uspf.num()) / static_cast<uint64_t>(uspf.den());
    auto dur = std::chrono::microseconds(uspf_int);
    return dur;
}
}  // namespace

using enum RootSt;

auto RootSm::step(st::tag_t<Init>) -> state_type {
    auto item = m_ctx->extract_buf();
    if (!item) return Init;
    return match(
        *std::move(item),
        [&](DecodingMeta meta) {
            m_ctx->task->set_period(meta_to_period(meta));
            if (meta.still())
                return Img;
            else
                return Vid;
        },
        [&](media::Frame frm) {
            m_ctx->prev_frm = std::exchange(m_ctx->frm, std::move(frm));
            PLAI_WARN("Got frame input while in Init state. Deducing as video");
            return Vid;
        });
}
auto RootSm::step(st::tag_t<Vid>) -> state_type {
    // No need to  check if frame is stored in m_ctx already since that is
    // rendered by the preceding blending state anyway.
    auto item = m_ctx->extract_buf();
    if (!item) {
        PLAI_WARN("Missing frame when rendering an image");
        return Vid;
    }
    return match(
        *std::move(item),
        [&](DecodingMeta meta) {
            if (meta.still()) return Vid2Img;
            m_ctx->task->set_period(meta_to_period(meta));
            return Vid2Vid;
        },
        [&](media::Frame frm) {
            m_ctx->prev_frm = std::exchange(m_ctx->frm, std::move(frm));
            m_ctx->text->update(m_ctx->frm);
            m_ctx->text->render_to(MAIN_TARGET);
            return Vid;
        });
}

auto RootSm::step(st::tag_t<Img>) -> state_type {
    if (m_img_sm.done()) {
        auto item = m_ctx->extract_buf();
        if (!item) {
            PLAI_WARN("No media available after image");
            return Img;
        }
        auto defer = Defer([&] { m_img_sm.reset(); });
        return match(
            *std::move(item),
            [&](DecodingMeta meta) {
                if (meta.still()) return Img2Img;
                m_ctx->task->set_period(meta_to_period(meta));
                return Img2Vid;
            },
            [&](media::Frame frm) {
                assert(frm);
                m_ctx->prev_frm = std::exchange(m_ctx->frm, std::move(frm));
                PLAI_WARN("Received a frame when expecting metadata");
                m_ctx->task->set_period(PERIOD_30MS);
                return Vid;
            });
    }
    m_img_sm();
    return Img;
}

auto RootSm::step(st::tag_t<Vid2Vid>) -> state_type {
    if (m_blend_sm.initial()) { m_blend_sm->setup(false, false); }
    m_blend_sm();
    if (m_blend_sm.done()) {
        m_blend_sm.reset();
        return Vid;
    }
    return Img2Vid;
}

auto RootSm::step(st::tag_t<Vid2Img>) -> state_type {
    if (m_blend_sm.initial()) { m_blend_sm->setup(false, true); }
    m_blend_sm();
    if (m_blend_sm.done()) {
        m_blend_sm.reset();
        return Vid;
    }
    return Img2Vid;
}

auto RootSm::step(st::tag_t<Img2Vid>) -> state_type {
    if (m_blend_sm.initial()) { m_blend_sm->setup(true, false); }
    m_blend_sm();
    if (m_blend_sm.done()) {
        m_blend_sm.reset();
        return Vid;
    }
    return Img2Vid;
}

auto RootSm::step(st::tag_t<Img2Img>) -> state_type {
    if (m_blend_sm.initial()) m_blend_sm->setup(true, true);
    m_blend_sm();
    if (m_blend_sm.done()) {
        m_blend_sm.reset();
        return Img;
    }
    return Img2Img;
}

}  // namespace plai::mods::player
