#include <condition_variable>
#include <deque>
#include <mutex>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <plai/modules/decoder.hpp>
#include <print>
#include <thread>

namespace plai::mod {

class DecoderImpl final : public Decoder {
 public:
    explicit DecoderImpl(FrameBuffer* fbuf) : m_fbuf(fbuf) {}
    void set_dimensions(Vec<int> dims) override {}
    void decode(std::vector<uint8_t> media) override {
        {
            std::unique_lock lk{m_mut};
            m_queue.push_back(std::move(media));
        }
        m_cv.notify_one();
    }

 private:
    void work(std::stop_token stok) {
        while (true) {
            if (!wait_job(stok)) break;
            auto demux = demux_for_first_in_queue();
            std::println("set up demux");
            auto [stream_idx, stream] = demux.best_video_stream();
            std::println("set up stream");
            auto decoder = media::Decoder(stream);
            std::println("set up decoder");
            auto pkt = plai::media::Packet();
            auto frm = m_fbuf->get_frame();
            std::println("extracting packet");
            while (!stok.stop_requested() && demux >> pkt) {
                std::println("got a packet");
                if (pkt.stream_index() != stream_idx) [[unlikely]]
                    continue;
                std::println("decoding packet");
                decoder << pkt;
                if (!(decoder >> frm)) continue;
                m_fbuf->push_frame(std::move(frm));
                if (frm.width() == 0) break;
                frm = m_fbuf->get_frame();
                std::println("decoded a frame");
            }
            {
                std::unique_lock lk{m_mut};
                m_queue.pop_front();
            }
        }
    }

    [[nodiscard]] bool wait_job(std::stop_token stok) {
        std::unique_lock lk{m_mut};
        m_cv.wait(lk,
                  [&] { return stok.stop_requested() || !m_queue.empty(); });
        return !stok.stop_requested();
    }

    [[nodiscard]] media::Demux demux_for_first_in_queue() {
        std::unique_lock lk{m_mut};
        return media::Demux(m_queue.front());
    }

    std::mutex m_mut{};
    std::condition_variable m_cv{};
    FrameBuffer* m_fbuf{};
    std::deque<std::vector<uint8_t>> m_queue{};
    std::jthread m_worker{[&](std::stop_token stok) { work(stok); }};
};

std::unique_ptr<Decoder> make_decoder(FrameBuffer* frm_buf) {
    return std::make_unique<DecoderImpl>(frm_buf);
}

}  // namespace plai::mod
