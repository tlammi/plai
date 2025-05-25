
#include <plai/exceptions.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media/frame_converter.hpp>

#include "av_check.hpp"

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

namespace plai::media {
namespace {

bool is_hardware_frame(AVPixelFormat fmt) {
    const auto* desc = av_pix_fmt_desc_get(fmt);
    if (!desc) return false;
    return desc->flags & AV_PIX_FMT_FLAG_HWACCEL;
}

/**
 * Convert deprecated YUVJ pixel format to standard YUV (+ adjust colorspace
 * later)
 * */
AVPixelFormat intermediate_pixel_fmt(AVPixelFormat in) {
    switch (in) {
        case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
        case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
        case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
        case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
        default: return in;
    }
}

/// Adjust colorspace after switching from YUVJ to YUV
void adjust_colorspace(SwsContext* ctx) {
    int src_range{};
    int dst_range{};
    int brightness{};
    int contrast{};
    int saturation{};
    int* dummy{};

    sws_getColorspaceDetails(ctx, &dummy, &src_range, &dummy, &dst_range,
                             &brightness, &contrast, &saturation);
    src_range = 1;
    const int* coefs = sws_getCoefficients(SWS_CS_DEFAULT);
    sws_setColorspaceDetails(ctx, coefs, src_range, coefs, dst_range,
                             brightness, contrast, saturation);
}

// modify the pixels so they are compatible with SDL
AVPixelFormat output_pixel_format(AVPixelFormat intermediate) {
    switch (intermediate) {
        case AV_PIX_FMT_NV12:
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUV440P:
        case AV_PIX_FMT_YUV444P: return AV_PIX_FMT_YUV420P;
        default: return intermediate;
    }
}
}  // namespace

FrameConverter::~FrameConverter() {
    if (m_ctx) sws_freeContext(m_ctx);
}

Frame FrameConverter::operator()(Vec<int> dst_dims, const Frame& src,
                                 Frame dst) {
    auto pix_fmt = static_cast<AVPixelFormat>(src.raw()->format);
    if (is_hardware_frame(pix_fmt)) {
        Frame tmp{};
        AV_CHECK(av_hwframe_transfer_data(tmp.raw(), src.raw(), 0));
        return (*this)(dst_dims, tmp, std::move(dst));
    }
    auto intermediate_fmt = intermediate_pixel_fmt(pix_fmt);
    auto out_pix_fmt = output_pixel_format(intermediate_fmt);
    PLAI_TRACE("Converting pixel format {} to {} via {}",
               av_get_pix_fmt_name(pix_fmt), av_get_pix_fmt_name(out_pix_fmt),
               av_get_pix_fmt_name(intermediate_fmt));
    auto src_dims = src.dims();
    m_ctx = sws_getCachedContext(m_ctx, src_dims.x, src_dims.y,
                                 intermediate_fmt, dst_dims.x, dst_dims.y,
                                 out_pix_fmt, 0, nullptr, nullptr, nullptr);
    if (!m_ctx) throw ValueError("Could not create libswscale context");
    if (pix_fmt != intermediate_fmt) adjust_colorspace(m_ctx);
    auto* raw = dst.raw();
    raw->format = out_pix_fmt;
    raw->width = dst_dims.x;
    raw->height = dst_dims.y;
    if (!dst.is_dynamic()) {
        av_image_alloc(raw->data, raw->linesize, dst_dims.x, dst_dims.y,
                       out_pix_fmt, 1);
        dst.is_dynamic(true);
    }
    sws_scale(m_ctx, (const uint8_t* const*)(src.raw()->data),
              src.raw()->linesize, 0, src.height(), raw->data, raw->linesize);
    return dst;
}

}  // namespace plai::media
