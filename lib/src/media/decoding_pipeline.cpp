#include <plai/fs/read.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/decoding_pipeline.hpp>
#include <plai/media/demux.hpp>
#include <plai/util/match.hpp>
#include <print>

namespace plai::media {
namespace {
auto demux_from_media(DecodingPipeline::Media& m) {
    return match(m, [](auto& arg) { return Demux(arg.data); });
}

}  // namespace
DecodingPipeline::~DecodingPipeline() {
    m_worker.request_stop();
    m_cv.notify_one();
    m_worker.join();
}

DecodingStream DecodingPipeline::frame_stream() {
    return {&m_buf, m_framerates.pop()};
}

void DecodingPipeline::decode_video(std::vector<uint8_t> data) {
    std::println("pushing video");
    {
        std::unique_lock lk{m_mut};
        m_medias.push_back(Video(std::move(data)));
    }
    m_cv.notify_one();
}
void DecodingPipeline::decode_image(std::vector<uint8_t> data) {
    std::println("pushing image");
    {
        std::unique_lock lk{m_mut};
        m_medias.push_back(Image(std::move(data)));
    }
    m_cv.notify_one();
}
void DecodingPipeline::decode(Media m) {
    {
        std::unique_lock lk{m_mut};
        m_medias.push_back(std::move(m));
    }
    m_cv.notify_one();
}

void DecodingPipeline::decode(stdfs::path path) {
    auto data = fs::read_bin(path);
    auto ext = plai::to_lower(std::string(path.extension()));
    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
        PLAI_DEBUG("deduced as image: {}", path.native());
        decode_image(std::move(data));
    } else {
        PLAI_DEBUG("deduced as video: {}", path.native());
        decode_video(std::move(data));
    }
}

size_t DecodingPipeline::queued_medias() const noexcept {
    auto lk = std::unique_lock(m_mut);
    return m_medias.size();
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
        bool is_img = std::holds_alternative<Image>(media);
        std::println("is_img: {}", is_img);
        auto demux = demux_from_media(media);
        auto [stream_idx, stream] = demux.best_video_stream();
        lk.lock();
        m_framerates.push(stream.fps());
        lk.unlock();
        auto decoder = Decoder(stream);
        auto pkt = Packet();
        auto frm = Frame();
        auto converted_frame = Frame();
        size_t decoded_frames = 0;
        while (!tok.stop_requested() && demux >> pkt) {
            if (pkt.stream_index() != stream_idx) continue;
            decoder << pkt;
            if (!(decoder >> frm)) continue;
            if (m_dims) {
                converted_frame =
                    m_conv(m_dims, frm, std::move(converted_frame));
                converted_frame = m_buf.push(std::move(converted_frame));
                std::swap(frm, converted_frame);
            } else {
                frm = m_buf.push(std::move(frm));
            }
            ++decoded_frames;
            if (is_img) break;
        }
        PLAI_DEBUG("decoded total {} frames", decoded_frames);
        // end of stream
        m_buf.push(Frame());
    }
}

}  // namespace plai::media
