
#include <chrono>
#include <format>
#include <plai/frontend/events.hpp>
#include <plai/frontend/frontend.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <plai/media/frame_converter.hpp>
#include <plai/ring_buffer.hpp>
#include <plai/time.hpp>
#include <print>
#include <thread>

using plai::media::Decoder;
using plai::media::Demux;
using plai::media::Frame;
using plai::media::FrameConverter;
using plai::media::Packet;

using namespace std::literals::chrono_literals;

void producer(std::stop_token tok, const char* path,
              plai::RingBuffer<Frame>* buf) {
    size_t counter = 0;
    auto defer = plai::Defer([] { std::println("producer done"); });
    auto demux = Demux(path);
    auto [stream_idx, stream] = demux.best_video_stream();
    auto decoder = Decoder(stream);
    auto pkt = Packet();
    auto frm = Frame();
    while (!tok.stop_requested() && (demux >> pkt)) {
        if (pkt.stream_index() != stream_idx) continue;
        decoder << pkt;
        if (!(decoder >> frm)) continue;
        std::println("producing");
        auto width = frm.width();
        buf->emplace(std::exchange(frm, {}));
        std::println("produced {}, {}", ++counter, width);
        if (!width) {
            std::println("was empty");
            return;
        }
    }
    buf->emplace(std::move(frm));
}

int main(int argc, char** argv) {
    try {
        if (argc != 2)
            throw std::runtime_error(
                std::format("usage: {} path/to/file", argv[0]));
        // plai::logs::init(plai::logs::Level::Trace);
        plai::logs::init(plai::logs::Level::Info);
        auto front = plai::frontend("sdl2");
        auto text = front->texture();
        auto queue = plai::RingBuffer<Frame>(60);
        size_t counter = 0;

        auto worker = std::jthread(&producer, argv[1], &queue);

        static constexpr auto SPF = 1.0 / 30.0;
        static constexpr auto spf =
            std::chrono::microseconds(int64_t(SPF * 1'000'000));
        auto rate_limiter = plai::RateLimiter(spf);
        while (true) {
            while (true) {
                auto event = front->poll_event();
                if (!event) break;
                if (std::holds_alternative<plai::Quit>(*event)) {
                    return EXIT_SUCCESS;
                }
            }
            std::println("consuming");
            auto f = queue.pop();
            std::println("consumed: {}, {}", ++counter, f.width());
            if (!f.width()) return EXIT_SUCCESS;
            text->update(f);
            text->render_to({});
            rate_limiter();
            front->render_current();
        }

    } catch (const std::exception& e) {
        std::println(stderr, "ERROR: {}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
