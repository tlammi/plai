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
bool StreamView::is_still_image() const noexcept {
    const auto type = m_raw->codecpar->codec_type;
    const auto id = m_raw->codecpar->codec_id;
    if (type != AVMEDIA_TYPE_VIDEO) return false;
    return id == AV_CODEC_ID_MJPEG || id == AV_CODEC_ID_JPEG2000;
}
}  // namespace plai::media
