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

}  // namespace plai::mods::player
