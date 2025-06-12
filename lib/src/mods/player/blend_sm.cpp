#include "blend_sm.hpp"

#include <plai/util/match.hpp>

namespace plai::mods::player {
namespace {
void set_blend_modes(Ctx& ctx, BlendMode bm) {
    ctx.text->blend_mode(bm);
    ctx.back->blend_mode(bm);
    for (auto& text : ctx.watermark_textures) { text->blend_mode(bm); }
}
}  // namespace

auto BlendSm::step(st::tag_t<Init>) -> state_type {
    auto item = m_ctx->extract_buf();
    if (!item) {
        PLAI_WARN("Could not extract frame for blending");
        return Done;
    }
    auto success = match(
        *std::move(item),
        [&](DecodingMeta meta) {
            PLAI_WARN("Received metadata when trying to blend");
            if (!meta.still()) { m_ctx->task->set_period(PERIOD_30MS); }
            return false;
        },
        [&](media::Frame frm) {
            assert(frm);
            m_ctx->prev_frm = std::exchange(m_ctx->frm, std::move(frm));
            return true;
        });
    if (!success) return Done;
    set_blend_modes(*m_ctx, BlendMode::Blend);

    m_tstamp = Clock::now();
    if (!m_src_img && m_dst_img) {
        m_alpha = AlphaCalc(WATERMARK_BLEND);
        return WatermarkFadeIn;
    }
    m_ctx->task->set_period(PERIOD_30MS);
    m_alpha = AlphaCalc(m_ctx->opts.blend_dur);
    return Blend;
}

auto BlendSm::step(st::tag_t<WatermarkFadeIn>) -> state_type {
    auto alpha = m_alpha();
    m_ctx->set_watermark_alpha(alpha);
    if (alpha == MAX_ALPHA) {
        m_alpha = AlphaCalc(m_ctx->opts.blend_dur);
        return Blend;
    }
    return WatermarkFadeIn;
}

auto BlendSm::step(st::tag_t<Blend>) -> state_type {
    auto alpha = m_alpha();
    assert(m_ctx->prev_frm);
    m_ctx->back->update(m_ctx->prev_frm);
    m_ctx->back->alpha(MAX_ALPHA - alpha);
    m_ctx->text->update(m_ctx->frm);
    m_ctx->text->alpha(alpha);
    if (alpha == MAX_ALPHA) {
        if (!m_src_img && m_dst_img) {
            m_alpha = AlphaCalc(WATERMARK_BLEND);
            return WatermarkFadeOut;
        }
        set_blend_modes(*m_ctx, BlendMode::None);
        return Done;
    }
    return Blend;
}

auto BlendSm::step(st::tag_t<WatermarkFadeOut>) -> state_type {
    auto alpha = m_alpha();
    m_ctx->set_watermark_alpha(MAX_ALPHA - alpha);
    if (alpha == MAX_ALPHA) {
        set_blend_modes(*m_ctx, BlendMode::None);
        return Done;
    }
    return WatermarkFadeOut;
}
}  // namespace plai::mods::player
