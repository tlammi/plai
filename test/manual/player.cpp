
#include <chrono>
#include <format>
#include <plai/frontend/events.hpp>
#include <plai/frontend/frontend.hpp>
#include <plai/fs/read.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/decoding_pipeline.hpp>
#include <plai/media/demux.hpp>
#include <plai/media/frame_converter.hpp>
#include <plai/persist_buffer.hpp>
#include <plai/time.hpp>
#include <plai/util/defer.hpp>
#include <print>
#include <thread>

using namespace std::literals::chrono_literals;

int main(int argc, char** argv) {
    if (argc != 2)
        throw std::runtime_error(
            std::format("usage: {} path/to/file", argv[0]));

    auto pline = plai::media::DecodingPipeline();
    auto data = plai::fs::read_bin(argv[1]);
    pline.decode(std::move(data));
    auto stream = pline.frame_stream();
    auto front = plai::frontend("sdl2");
    auto text = front->texture();
    for (const auto& frm : stream) {
        while (true) {
            auto event = front->poll_event();
            if (!event) break;
            if (std::holds_alternative<plai::Quit>(*event)) {
                return EXIT_SUCCESS;
            }
        }
        text->update(frm);
        text->render_to({});
        front->render_current();
        std::this_thread::sleep_for(10ms);
    }
}
