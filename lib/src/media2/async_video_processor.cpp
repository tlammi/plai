#include <plai/logs/logs.hpp>
#include <plai/media/frame_converter.hpp>
#include <plai/media2/async_video_processor.hpp>

namespace plai::media2 {
namespace {

struct Processor {
    ProcessingCtx* ctx;
    media::Demux demux;
    std::pair<size_t, media::StreamView> stream;
    media::Decoder decoder;
    media::FrameConverter m_conv{};

    Processor(ProcessingCtx* ctx)
        : ctx(ctx),
          demux(ctx->data()),
          stream(ctx->select_stream(demux)),
          decoder(stream.second, ctx->hwaccel()) {}

    exec::task<void> process() {
        bool is_still = stream.second.is_still_image();
        auto fps = stream.second.fps();
        ctx->meta(is_still, fps);
        if (is_still)
            co_await process_still();
        else
            co_await process_vid();
    }

    exec::task<void> process_still() {
        auto pkt = media::Packet();
        auto frm = media::Frame();
        auto real_frm = media::Frame();
        auto dims = ctx->dims();
        while (demux >> pkt) {
            if (pkt.stream_index() != stream.first) continue;
            decoder << pkt;
            if (!(decoder >> frm)) continue;
            if (frm.width() > real_frm.width()) {
                // select the best frame
                real_frm = std::exchange(frm, {});
            }
        }
        if (dims) {
            auto input_dims = real_frm.dims();
            input_dims.scale_to(dims);
            ctx->frame(m_conv(input_dims, std::move(real_frm)));
        }
        co_return;
    }

    exec::task<void> process_vid() {
        size_t decoded_frames = 0;
        auto pkt = media::Packet();
        auto frm = media::Frame();
        auto dims = ctx->dims();
        while (demux >> pkt) {
            if (pkt.stream_index() != stream.first) continue;
            decoder << pkt;
            if (!(decoder >> frm)) continue;
            if (dims) {
                auto input_dims = frm.dims();
                input_dims.scale_to(dims);
                ctx->frame(m_conv(input_dims, std::exchange(frm, {})));
            } else
                // TODO: This will break things. Luckily m_dims is always
                // set
                ctx->frame(std::exchange(frm, {}));
            ++decoded_frames;
            co_await ex::yield();
        }
        PLAI_DEBUG("decoded total {} frames", decoded_frames);

        co_return;
    }
};

}  // namespace

exec::task<void> process_video(ProcessingCtx* ctx) {
    assert(ctx);

    auto proc = Processor(ctx);
    co_await proc.process();
}

}  // namespace plai::media2
