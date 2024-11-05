#include <libavutil/pixfmt.h>

#include <plai/exceptions.hpp>
#include <plai/media/frame_converter.hpp>

extern "C" {
#include <libswscale/swscale.h>
}

namespace plai::media {
namespace {
AVPixelFormat pixel_conversion(AVPixelFormat in) {
    switch (in) {
        case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV420P;
        default: return in;
    }
}
}  // namespace

FrameConverter::~FrameConverter() {
    if (m_ctx) sws_freeContext(m_ctx);
}

Frame FrameConverter::operator()(Vec<int> dst_dims, const Frame& src,
                                 Frame dst) {
    auto pix_fmt = static_cast<AVPixelFormat>(src.raw()->format);
    auto out_pix_fmt = pixel_conversion(pix_fmt);
    auto src_dims = src.dims();
    m_ctx = sws_getCachedContext(m_ctx, src_dims.x, src_dims.y, pix_fmt,
                                 dst_dims.x, dst_dims.y, out_pix_fmt, 0,
                                 nullptr, nullptr, nullptr);
    if (!m_ctx) throw ValueError("Could not create libswscale context");
    auto* raw = dst.raw();
    raw->format = out_pix_fmt;
    raw->width = dst_dims.x;
    raw->height = dst_dims.y;
    av_frame_get_buffer(raw, 0);
    sws_scale(m_ctx, (const uint8_t* const*)(src.raw()->data),
              src.raw()->linesize, 0, src.height(), raw->data, raw->linesize);
    return dst;
}

}  // namespace plai::media
