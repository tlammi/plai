#include <plai/media/stream_view.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace plai::media {

bool StreamView::video() const noexcept {
    return m_raw->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
}

bool StreamView::audio() const noexcept {
    return m_raw->codecpar->codec_type == AVMEDIA_TYPE_AUDIO;
}

Frac<int> StreamView::fps() const noexcept {
    auto num = m_raw->r_frame_rate.num;
    auto den = m_raw->r_frame_rate.den;
    return {num, den};
}
}  // namespace plai::media
