#include "ctx.hpp"

#include <print>

namespace plai::mods::player {

std::optional<Decoded> Ctx::extract_buf() {
    auto lk = std::unique_lock(mut);
    auto item = std::exchange(buf, std::nullopt);
    lk.unlock();
    if (!item) return item;
    notify_sink_ready();
    auto* frm = std::get_if<media::Frame>(&*item);
    if (frm) {
        auto dims = frm->dims();
        dims.scale_to(this->dims);
        *frm = m_conv(dims, *frm);
    }
    return item;
}

void Ctx::render_watermarks(uint8_t alpha) {
    for (size_t i = 0; i < watermark_textures.size(); ++i) {
        auto& text = watermark_textures.at(i);
        auto& watermark = opts.watermarks.at(i);
        text->alpha(alpha);
        text->render_to(watermark.target);
    }
}
}  // namespace plai::mods::player
