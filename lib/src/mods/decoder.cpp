#include <plai/logs/logs.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <plai/media/hwaccel.hpp>
#include <plai/mods/decoder.hpp>
#include <plai/sched/task.hpp>
#include <plai/util/memfn.hpp>

namespace plai::mods {

class DecoderImpl final : public Decoder {
    using flow::Src<Decoded>::notify_src_ready;
    using flow::Sink<media::Media>::notify_sink_ready;

 public:
    explicit DecoderImpl(sched::Executor exec) : m_exec(std::move(exec)) {}

    void consume(media::Media media) override {
        {
            auto lk = std::lock_guard(m_mut);
            m_media = std::move(std::get<media::Image>(std::move(media)));
            PLAI_DEBUG("Decoder consuming media with size {}",
                       m_media->data.size());
        }
        sched::post(m_exec, memfn(this, &DecoderImpl::launch_decoding));
    }

    bool sink_ready() override {
        auto lk = std::lock_guard(m_mut);
        return !m_media.has_value();
    }

    Decoded produce() override {
        PLAI_TRACE("Decoder producing a frame");
        return m_frame_buf.pop();
    }
    bool src_ready() override { return !m_frame_buf.empty(); }

 private:
    void launch_decoding() {
        auto lk = std::unique_lock(m_mut);
        m_demux.emplace(m_media->data);
        m_decoded_frames = 0;
        lk.unlock();
        std::tie(m_stream_idx, m_stream) = m_demux->best_video_stream();
        auto still = m_stream.is_still_image();
        m_decoder = media::Decoder(m_stream);
        if (still) {
            m_frame_buf.emplace(DecodingMeta{.fps = NaN<int>});
            m_still_decode.post();
        } else {
            m_frame_buf.emplace(DecodingMeta{.fps = m_stream.fps()});
            m_video_decode.post();
        }
        notify_src_ready();
    }

    // Still image might have metadata like icons which needs to be discarded so
    // we just select the frame with the largest width.
    //
    // This could be optimized a bit to run in multiple steps if this takes too
    // long.
    void decode_step_still() {
        auto pkt = media::Packet();
        auto frm = media::Frame();
        auto real_frm = media::Frame();
        while (*m_demux >> pkt) {
            if (pkt.stream_index() != m_stream_idx) continue;
            m_decoder << pkt;
            if (!(m_decoder >> frm)) continue;
            if (frm.width() > real_frm.width())
                real_frm = std::exchange(frm, {});
        }
        m_frame_buf.push(std::move(real_frm));
        {
            auto lk = std::lock_guard(m_mut);
            m_media.reset();
        }
        notify_src_ready();
        notify_src_ready();
    }

    void decode_step() {
        auto pkt = media::Packet();
        auto frm = media::Frame();
        while (*m_demux >> pkt) {
            if (pkt.stream_index() != m_stream_idx) continue;
            m_decoder << pkt;
            if (!(m_decoder >> frm)) continue;
            m_frame_buf.push(std::move(frm));
            notify_src_ready();
            ++m_decoded_frames;
            m_video_decode.post();
            return;
        }
        PLAI_DEBUG("decoded total {} frames", m_decoded_frames);
        m_media.reset();
        notify_sink_ready();
    }

    std::mutex m_mut{};
    sched::Executor m_exec;

    std::optional<media::Demux> m_demux{};
    unsigned long m_stream_idx{};
    media::StreamView m_stream{};
    media::Decoder m_decoder{};
    size_t m_decoded_frames{};

    sched::Task m_video_decode = sched::task() | sched::executor(m_exec) |
                                 memfn(this, &DecoderImpl::decode_step) |
                                 sched::task_finish();
    sched::Task m_still_decode = sched::task() | sched::executor(m_exec) |
                                 memfn(this, &DecoderImpl::decode_step_still) |
                                 sched::task_finish();

    std::optional<media::Image> m_media{};
    static constexpr size_t FRAME_BUFFER_SIZE = 10;
    RingBuffer<Decoded> m_frame_buf{FRAME_BUFFER_SIZE};
    media::HwAccel m_accel{};
};

std::unique_ptr<Decoder> make_decoder(sched::Executor exec) {
    return std::make_unique<DecoderImpl>(std::move(exec));
}
}  // namespace plai::mods
