#include <plai/fs/read.hpp>
#include <plai/play/player.hpp>
#include <plai/util/defer.hpp>
#include <plai/util/str.hpp>
#include <print>

struct Playlist final : public plai::play::MediaSrc {
    std::filesystem::path path;

    Playlist(std::filesystem::path p) : path(std::move(p)) {}

    std::optional<plai::media::Media> next_media() final {
        if (path.empty()) return std::nullopt;
        plai::Defer defer{[&] { path.clear(); }};
        auto ext = plai::to_lower(path.extension());
        if (ext == ".jpeg" || ext == ".jpg" || ext == ".png") {
            std::println("reading image: {}", path.native());
            return plai::media::Image(plai::fs::read_bin(path));
        } else {
            std::println("reading video: {}", path.native());
            return plai::media::Video(plai::fs::read_bin(path));
        }
    }
};

int main(int argc, char** argv) {
    if (argc != 2)
        throw std::runtime_error(
            std::format("usage: {} path/to/file", argv[0]));
    auto front = plai::frontend("sdl2");
    Playlist plist{std::filesystem::path(argv[1])};
    auto player =
        plai::play::Player(front.get(), &plist, {.wait_media = false});
    player.run();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
#if 0
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
    pline.set_dims({1920, 1080});
    pline.decode(argv[1]);
    auto stream = pline.frame_stream();
    auto front = plai::frontend("sdl2");
    auto text = front->texture();
    auto fps = stream.fps();
    auto dur = std::chrono::microseconds(
        static_cast<uint64_t>(1'000'000 / static_cast<double>(fps)));
    auto sleeper = plai::RateLimiter(dur);
    size_t frame_counter = 0;
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
        std::this_thread::sleep_for(1s);
        sleeper();
        ++frame_counter;
    }
    std::println("frame counter: {}", frame_counter);
    if (frame_counter == 1) { std::this_thread::sleep_for(3s); }
}
#endif
