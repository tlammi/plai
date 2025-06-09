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
        m_ready = false;
        sched::post(
            m_exec,
            [&, m = std::move(std::get<media::Image>(media))]() mutable {
                decode_dispatch(std::move(m));
            });
    }

    bool ready() override { return m_ready; }

    void on_sink_ready(flow::SinkSubscriber* sub) override { m_sink_sub = sub; }

    Decoded produce() override { return m_frame_buf.pop(); }
    bool data_available() override { return !m_frame_buf.empty(); }

    void on_data_available(flow::SrcSubscriber* sub) override {
        m_src_sub = sub;
    }

 private:
    void decode_dispatch(media::Image media) {
        auto demux = media::Demux(media.data);
        auto [stream_idx, stream] = demux.best_video_stream();
        bool still = stream.is_still_image();
        auto decoder = media::Decoder(stream, m_accel);
        auto pkt = media::Packet();
        auto frm = media::Frame();
        push_buf(DecodingMeta{.fps = stream.fps()});
        if (still) {
            auto real_frm = media::Frame();
            while (demux >> pkt) {
                if (pkt.stream_index() != stream_idx) continue;
                decoder << pkt;
                if (!(decoder >> frm)) continue;
                if (frm.width() > real_frm.width())
                    real_frm = std::exchange(frm, {});
            }
            push_buf(std::move(real_frm));
        } else {
            size_t decoded_frames = 0;
            while (demux >> pkt) {
                if (pkt.stream_index() != stream_idx) continue;
                decoder << pkt;
                if (!(decoder >> frm)) continue;
                push_buf(std::move(frm));
                ++decoded_frames;
            }
            PLAI_DEBUG("decoded total {} frames", decoded_frames);
        }
    }

    void push_buf(auto&&... args) {
        auto size = m_frame_buf.size();
        m_frame_buf.emplace(std::forward<decltype(args)>(args)...);
        if (!size && m_src_sub) m_src_sub->src_data_available();
    }

    sched::Executor m_exec;
    static constexpr size_t FRAME_BUFFER_SIZE = 10;
    RingBuffer<Decoded> m_frame_buf{FRAME_BUFFER_SIZE};
    media::HwAccel m_accel{};
    flow::SrcSubscriber* m_src_sub{};
    flow::SinkSubscriber* m_sink_sub{};
    std::atomic_bool m_ready{};
};

std::unique_ptr<Decoder> make_decoder(sched::Executor exec) {
    return std::make_unique<DecoderImpl>(std::move(exec));
}
}  // namespace plai::mods
