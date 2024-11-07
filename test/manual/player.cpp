
#include <chrono>
#include <format>
#include <plai/frontend/events.hpp>
#include <plai/frontend/frontend.hpp>
#include <plai/fs/read.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <plai/media/frame_converter.hpp>
#include <plai/media/pipeline.hpp>
#include <plai/persist_buffer.hpp>
#include <plai/time.hpp>
#include <plai/util/defer.hpp>
#include <print>
#include <thread>

using plai::media::Decoder;
using plai::media::Demux;
using plai::media::Frame;
using plai::media::FrameConverter;
using plai::media::Packet;

using namespace std::literals::chrono_literals;

void producer(std::stop_token tok, const char* path,
              plai::PersistBuffer<Frame>* buf) {
    auto defer = plai::Defer([] { std::println("producer done"); });
    // auto data = plai::fs::read_bin(path);
    // auto demux = Demux(data);
    auto demux = Demux(path);
    auto [stream_idx, stream] = demux.best_video_stream();
    auto decoder = Decoder(stream);
    auto pkt = Packet();
    auto frm = Frame();
    auto converter = plai::media::FrameConverter();
    while (!tok.stop_requested() && (demux >> pkt)) {
        if (pkt.stream_index() != stream_idx) continue;
        decoder << pkt;
        if (!(decoder >> frm)) continue;
        auto width = frm.width();
        if (width > 1920) { frm = converter({1920, 1080}, frm); }
        std::println("pushing frame to buffer");
        frm = buf->push(std::move(frm));
        if (!width) { return; }
    }
    buf->push({});
}

int main(int argc, char** argv) {
#if 0
    if (argc != 2)
        throw std::runtime_error(
            std::format("usage: {} path/to/file", argv[0]));

    std::println("starting");
    auto pipeline = plai::media::DecodingPipeline();
    auto data = plai::fs::read_bin(argv[1]);
    pipeline.push_media(std::move(data));
    auto stream = pipeline.next_stream();
    std::println("got fps: {}", double(stream.fps()));
    auto front = plai::frontend("sdl2");
    auto text = front->texture();

    for (auto& frame : stream) {
        while (true) {
            auto event = front->poll_event();
            if (!event) break;
            if (std::holds_alternative<plai::Quit>(*event)) {
                return EXIT_SUCCESS;
            }
        }
        text->update(frame);
        text->render_to({});
        front->render_current();
    }
#endif
    // try {
    plai::logs::init(plai::logs::Level::Trace);
    // plai::logs::init(plai::logs::Level::Info);
    auto front = plai::frontend("sdl2");
    auto text = front->texture();
    auto queue = plai::PersistBuffer<Frame>(60);
    size_t counter = 0;

    auto worker = std::jthread(&producer, argv[1], &queue);

    static constexpr auto SPF = 1.0 / 30.0;
    static constexpr auto spf =
        std::chrono::microseconds(int64_t(SPF * 1'000'000));
    auto rate_limiter = plai::RateLimiter(spf);
    auto f = plai::media::Frame();

    while (true) {
        std::println("counter: {}", counter);
        while (true) {
            auto event = front->poll_event();
            if (!event) break;
            if (std::holds_alternative<plai::Quit>(*event)) {
                return EXIT_SUCCESS;
            }
        }
        f = queue.pop(std::move(f));
        if (!f.width()) {
            if (counter == 1) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            return EXIT_SUCCESS;
        }
        text->update(f);
        text->render_to({});
        rate_limiter();
        front->render_current();
        ++counter;
    }
    // } catch (const std::exception& e) {
    //     std::println("ERROR: {}", e.what());
    //     return EXIT_FAILURE;
    // }
    return EXIT_SUCCESS;
}
