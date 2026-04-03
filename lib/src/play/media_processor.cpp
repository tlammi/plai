#include "media_processor.hpp"

#include <plai/exceptions.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>

namespace plai::play {

bool MediaProcessor::consume_next() {
    if (!m_processing) {
        PLAI_TRACE("Starting to consume a new media");
        auto meta = m_meta.try_pop();
        if (!meta) return false;
        auto fps = meta->still ? Frac<int>{} : meta->fps;
        m_out->new_media(m_buf.pop(), meta->still, fps);
        m_processing = true;
        return true;
    }
    auto frm = m_buf.pop();
    if (!frm) {
        m_processing = false;
        m_out->media_end_reached();
        return true;
    }
    m_out->new_frame(std::move(frm));
    return true;
}

void MediaProcessor::work(std::stop_token st) {
    while (true) {
        if (st.stop_requested()) return;
        try {
            auto media = m_in->next_media();
            PLAI_DEBUG("Processing next media");
            auto demux = media::Demux(media.data());
            auto [stream_idx, stream] = demux.best_video_stream();
            bool still = stream.is_still_image();
            PLAI_TRACE("Publishing new media meta");
            m_meta.push({.fps = stream.fps(), .still = still});
            auto decoder = media::Decoder(stream, m_accel);
            auto pkt = media::Packet();
            auto frm = media::Frame();
            if (stream.is_still_image()) {
                auto real_frm = media::Frame();
                while (!st.stop_requested() && demux >> pkt) {
                    if (pkt.stream_index() != stream_idx) continue;
                    decoder << pkt;
                    if (!(decoder >> frm)) continue;
                    if (frm.width() > real_frm.width())
                        real_frm = std::exchange(frm, {});
                }
                auto input_dims = real_frm.dims();
                input_dims.scale_to(m_dims);
                m_buf.push(m_conv(input_dims, std::move(real_frm)));
            } else {
                size_t decoded_frames = 0;
                while (!st.stop_requested() && demux >> pkt) {
                    if (pkt.stream_index() != stream_idx) continue;
                    decoder << pkt;
                    if (!(decoder >> frm)) continue;
                    if (m_dims) {
                        auto input_dims = frm.dims();
                        input_dims.scale_to(m_dims);
                        m_buf.push(m_conv(input_dims, std::exchange(frm, {})));
                    } else
                        // TODO: This will break things. Luckily m_dims is
                        // always set
                        m_buf.push(std::exchange(frm, {}));

                    ++decoded_frames;
                }
                PLAI_DEBUG("decoded total {} frames", decoded_frames);
            }
            m_buf.push({});
        } catch (const Cancelled&) {}
    }
}

}  // namespace plai::play
