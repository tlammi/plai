#include "blend_sm.hpp"

namespace plai::mods::player {
namespace {
void set_blend_states(Ctx& ctx, BlendMode bm) {
    ctx.text->blend_mode(bm);
    ctx.back->blend_mode(bm);
    for (auto& text : ctx.watermark_textures) { text->blend_mode(bm); }
}
}  // namespace

auto BlendSm::step(st::tag_t<Init>) -> state_type {
    set_blend_states(*m_ctx, BlendMode::Blend);

    m_tstamp = Clock::now();
    if (!m_src_img && m_dst_img) {
        m_alpha = AlphaCalc(WATERMARK_BLEND);
        return WatermarkFadeIn;
    }
    return Blend;
}

auto BlendSm::step(st::tag_t<WatermarkFadeIn>) -> state_type {
    auto alpha = m_alpha();
    m_ctx->back->update(m_ctx->prev_frm);
    m_ctx->render_watermarks(alpha);
    if (alpha == MAX_ALPHA) {
        m_alpha = AlphaCalc(m_ctx->opts.blend_dur);
        return Blend;
    }
    return WatermarkFadeIn;
}

auto BlendSm::step(st::tag_t<Blend>) -> state_type {
    auto alpha = m_alpha();
    m_ctx->back->update(m_ctx->prev_frm);
    m_ctx->back->alpha(MAX_ALPHA - alpha);
    m_ctx->back->render_to(MAIN_TARGET);
    m_ctx->text->update(m_ctx->frm);
    m_ctx->text->alpha(alpha);
    m_ctx->text->render_to(MAIN_TARGET);
    if (alpha == MAX_ALPHA) {
        if (!m_src_img && m_dst_img) {
            m_alpha = AlphaCalc(WATERMARK_BLEND);
            return WatermarkFadeOut;
        }
        set_blend_states(*m_ctx, BlendMode::None);
        return Done;
    }
    return Blend;
}

auto BlendSm::step(st::tag_t<WatermarkFadeOut>) -> state_type {
    auto alpha = m_alpha();
    m_ctx->render_watermarks(MAX_ALPHA - alpha);
    if (alpha == MAX_ALPHA) {
        set_blend_states(*m_ctx, BlendMode::None);
        return Done;
    }
    return WatermarkFadeOut;
}
}  // namespace plai::mods::player
