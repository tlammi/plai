#include <plai/media/decoder.hpp>
#include <plai/media/decoding_pipeline.hpp>
#include <plai/media/demux.hpp>

namespace plai::media {
DecodingPipeline::~DecodingPipeline() {
    m_worker.request_stop();
    m_cv.notify_one();
    m_worker.join();
}

DecodingStream DecodingPipeline::frame_stream() { return {&m_buf, {0, 0}}; }

void DecodingPipeline::decode(std::vector<uint8_t> data) {
    {
        std::unique_lock lk{m_mut};
        m_medias.push_back(std::move(data));
    }
    m_cv.notify_one();
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
        auto decoder = Decoder(stream);
        auto pkt = Packet();
        auto frm = Frame();
        while (!tok.stop_requested() && demux >> pkt) {
            decoder << pkt;
            if (!(decoder >> frm)) continue;
            frm = m_buf.push(std::move(frm));
        }
        // end of stream
        m_buf.push(Frame());
    }
}

}  // namespace plai::media
