#include <plai/logs/logs.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <plai/media/hwaccel.hpp>
#include <plai/mods/decoder.hpp>

namespace plai::mods {

class DecoderImpl final : public Decoder {
 public:
    explicit DecoderImpl(sched::Executor exec) : m_exec(std::move(exec)) {}

    void consume(media::Media media) override {
        auto lk = std::lock_guard(m_mut);
        m_buf.emplace(std::move(std::get<media::Image>(media)));
    }

    bool ready() override {
        auto lk = std::lock_guard(m_mut);
        return !m_buf.has_value();
    }

    void on_sink_ready(flow::SinkSubscriber* sub) override {}

    Decoded produce() override { return m_frame_buf.pop(); }
    bool data_available() override { return !m_frame_buf.empty(); }

    void on_data_available(flow::SrcSubscriber* sub) override {}

 private:
    void decode() {
        auto media = *std::exchange(m_buf, std::nullopt);
        auto demux = media::Demux(media.data);
        auto [stream_idx, stream] = demux.best_video_stream();
        bool still = stream.is_still_image();
        auto decoder = media::Decoder(stream, m_accel);
        auto pkt = media::Packet();
        auto frm = media::Frame();
        m_frame_buf.push(DecodingMeta{.fps = stream.fps()});
        if (still) {
            auto real_frm = media::Frame();
            while (demux >> pkt) {
                if (pkt.stream_index() != stream_idx) continue;
                decoder << pkt;
                if (!(decoder >> frm)) continue;
                if (frm.width() > real_frm.width())
                    real_frm = std::exchange(frm, {});
            }
            m_frame_buf.push(std::move(real_frm));
        } else {
            size_t decoded_frames = 0;
            while (demux >> pkt) {
                if (pkt.stream_index() != stream_idx) continue;
                decoder << pkt;
                if (!(decoder >> frm)) continue;
                m_frame_buf.push(std::move(frm));
                ++decoded_frames;
            }
            PLAI_DEBUG("decoded total {} frames", decoded_frames);
        }
    }

    std::mutex m_mut{};
    sched::Executor m_exec;
    std::optional<media::Image> m_buf{};
    static constexpr size_t FRAME_BUFFER_SIZE = 10;
    RingBuffer<Decoded> m_frame_buf{FRAME_BUFFER_SIZE};
    media::HwAccel m_accel{};
};

std::unique_ptr<Decoder> make_decoder(sched::Executor exec) {
    return std::make_unique<DecoderImpl>(std::move(exec));
}
}  // namespace plai::mods
