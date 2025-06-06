#include <plai/fs/read.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/decoding_pipeline.hpp>
#include <plai/media/demux.hpp>
#include <plai/util/match.hpp>

#include "plai/format.hpp"

namespace plai::media {

DecodingPipeline::~DecodingPipeline() {
    m_worker.request_stop();
    m_cv.notify_one();
    // if the buffer is full the producer thread gets stuck writing to the ring
    // buffer
    // Two reads is probably enough since the producer writes an additional null
    // frame to the buffer to indicate end of media, but what the hell.
    m_buf.try_pop();
    m_buf.try_pop();
    m_buf.try_pop();
    m_buf.try_pop();
    m_buf.try_pop();
    m_worker.join();
}

DecodingStream DecodingPipeline::frame_stream() {
    auto meta = m_metas.pop();
    return {&m_buf, std::move(meta.fps), meta.still};
}

void DecodingPipeline::decode(std::vector<uint8_t> data) {
    {
        std::unique_lock lk{m_mut};
        m_medias.push_back(std::move(data));
    }
    m_cv.notify_one();
}

void DecodingPipeline::decode(stdfs::path path) { decode(fs::read_bin(path)); }

size_t DecodingPipeline::queued_medias() const noexcept {
    auto lk = std::unique_lock(m_mut);
    return m_medias.size();
}
void DecodingPipeline::clear_media_queue() {
    auto lk = std::unique_lock(m_mut);
    m_medias.clear();
}

void DecodingPipeline::work(std::stop_token tok) {
    while (true) {
        std::unique_lock lk{m_mut};
        m_cv.wait(lk,
                  [&] { return tok.stop_requested() || !m_medias.empty(); });
        if (tok.stop_requested()) break;
        auto media = std::move(m_medias.front());
        m_medias.pop_front();
        lk.unlock();
        auto demux = Demux(media);
        auto [stream_idx, stream] = demux.best_video_stream();
        bool still = stream.is_still_image();
        lk.lock();
        m_metas.push({.fps = stream.fps(), .still = still});
        lk.unlock();
        auto decoder = Decoder(stream, m_accel);
        auto pkt = Packet();
        auto frm = Frame();
        if (stream.is_still_image()) {
            auto real_frm = Frame();
            while (!tok.stop_requested() && demux >> pkt) {
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
            while (!tok.stop_requested() && demux >> pkt) {
                if (pkt.stream_index() != stream_idx) continue;
                decoder << pkt;
                if (!(decoder >> frm)) continue;
                if (m_dims) {
                    auto input_dims = frm.dims();
                    input_dims.scale_to(m_dims);
                    m_buf.push(m_conv(input_dims, std::exchange(frm, {})));
                } else
                    // TODO: This will break things. Luckily m_dims is always
                    // set
                    m_buf.push(std::exchange(frm, {}));

                ++decoded_frames;
            }
            PLAI_DEBUG("decoded total {} frames", decoded_frames);
        }
        // end of stream
        m_buf.push(Frame());
    }
}

}  // namespace plai::media
