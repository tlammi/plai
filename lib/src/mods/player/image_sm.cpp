#include "image_sm.hpp"

#include <plai/util/match.hpp>

namespace plai::mods::player {

auto ImageSm::step(st::tag_t<Init>) -> state_type {
    if (!m_ctx->frm) {
        // No frame to show yet. Need to populate. This should only happen at
        // startup.
        auto item = m_ctx->extract_buf();
        if (!item) return Init;
        return match(
            *std::move(item),
            [&](DecodingMeta meta) {
                PLAI_WARN("Duplicate decoding metadata");
                return Done;
            },
            [&](media::Frame frm) {
                m_ctx->prev_frm = std::exchange(m_ctx->frm, std::move(frm));
                return Show;
            });
    }
    return Show;
}

auto ImageSm::step(st::tag_t<Show>) -> state_type {
    m_ctx->text->update(m_ctx->frm);
    m_ctx->text->render_to(MAIN_TARGET);
    m_tstamp = Clock::now();
    return Delay;
}

auto ImageSm::step(st::tag_t<Delay>) -> state_type {
    m_ctx->text->render_to(MAIN_TARGET);
    if (Clock::now() - m_tstamp >= m_ctx->opts.image_dur) return End;
    return Delay;
}

auto ImageSm::step(st::tag_t<End>) -> state_type {
    m_ctx->prev_frm = std::exchange(m_ctx->frm, {});
    return Done;
}

}  // namespace plai::mods::player
