#include <libavutil/pixfmt.h>

#include <plai/media/frame_converter.hpp>

extern "C" {
#include <libswscale/swscale.h>
}

namespace plai::media {
namespace frame_converter_detail {
namespace {}

}  // namespace frame_converter_detail

FrameConverter::FrameConverter(Vec<int> dst, bool adjust_pixels)
    : m_ctx(nullptr, sws_freeContext),
      m_dst{dst},
      m_pix_adjust(adjust_pixels) {}

Frame FrameConverter::operator()(const Frame& input) {
    auto pix_fmt = static_cast<AVPixelFormat>(input.raw()->format);
    auto out_pix_fmt = pix_fmt;
    if (!m_ctx) {
        if (m_pix_adjust) {
            if (pix_fmt == AV_PIX_FMT_YUVJ422P) {
                out_pix_fmt = AV_PIX_FMT_YUV420P;
            }
        }
        m_ctx = std::unique_ptr<SwsContext, void (*)(SwsContext*)>(
            sws_getContext(input.width(), input.height(), pix_fmt, m_dst.x,
                           m_dst.y, out_pix_fmt, 0, nullptr, nullptr, nullptr),
            sws_freeContext);
    }
    auto out = Frame();
    auto* raw = out.raw();
    raw->format = out_pix_fmt;
    raw->width = m_dst.x;
    raw->height = m_dst.y;
    av_frame_get_buffer(raw, 0);
    sws_scale(m_ctx.get(), (const uint8_t* const*)(input.raw()->data),
              input.raw()->linesize, 0, input.height(), raw->data,
              raw->linesize);
    return out;
}

}  // namespace plai::media
