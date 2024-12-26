#include <filesystem>
#include <plai/fs/read.hpp>
#include <plai/play/player.hpp>
#include <plai/util/defer.hpp>
#include <plai/util/str.hpp>
#include <print>

using namespace std::literals::chrono_literals;

struct Playlist final : public plai::play::MediaSrc {
    std::span<const char* const> args;

    Playlist(std::span<const char* const> args) : args(args) {}

    std::optional<plai::media::Media> next_media() final {
        if (args.empty()) return std::nullopt;
        plai::Defer defer{[&] { args = args.subspan(1); }};
        auto path = std::filesystem::path(args[0]);
        auto ext = plai::to_lower(path.extension());
        if (ext == ".jpeg" || ext == ".jpg" || ext == ".png") {
            std::println("reading image {}", path.native());
            return plai::media::Image(plai::fs::read_bin(path));
        } else {
            std::println("reading video {}", path.native());
            return plai::media::Video(plai::fs::read_bin(path));
        }
    }
};

int main(int argc, char** argv) {
    if (argc < 2)
        throw std::runtime_error(
            std::format("usage: {} path/to/file...", argv[0]));
    auto front = plai::frontend("sdl2");
    Playlist plist{std::span<const char* const>(&argv[1], argc - 1)};
    auto player = plai::play::Player(front.get(), &plist,
                                     {.blend_dur = 1s, .wait_media = false});
    player.run();
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

enum class Mode {
    Play,
    SetupBlend,
    Blend,
    FinishBlend,
};

int main(int argc, char** argv) {
    if (argc < 2)
        throw std::runtime_error(
            std::format("usage: {} path/to/file...", argv[0]));

    auto pline = plai::media::DecodingPipeline();
    pline.set_dims({1920, 1080});
    for (size_t i = 1; i < argc; ++i) {
        // pline.decode(plai::fs::read_bin(argv[i]));
        pline.decode(argv[i]);
    }
    auto front = plai::frontend("sdl2");
    auto prev_frm = plai::media::Frame();
    for (size_t i = 1; i < argc; ++i) {
        auto stream = pline.frame_stream();
        auto text = front->texture();
        auto fading_text = front->texture();
        auto fps = stream.fps();
        auto dur = std::chrono::microseconds(
            static_cast<uint64_t>(1'000'000 / static_cast<double>(fps)));
        auto sleeper = plai::RateLimiter(dur);
        size_t frame_counter = 0;

        auto mode = i > 1 ? Mode::SetupBlend : Mode::Play;
        static constexpr uint8_t alpha_delta = 10;
        static constexpr uint8_t alpha_max =
            std::numeric_limits<uint8_t>::max();
        uint8_t alpha = 0;
        auto iter = stream.begin();
        while (iter != stream.end()) {
            auto& frm = *iter;
            while (true) {
                auto event = front->poll_event();
                if (!event) break;
                if (std::holds_alternative<plai::Quit>(*event)) {
                    return EXIT_SUCCESS;
                }
            }
            using enum Mode;
            switch (mode) {
                case SetupBlend: {
                    std::println("blend setup");
                    fading_text->update(prev_frm);
                    text->update(frm);
                    fading_text->blend_mode(plai::BlendMode::Blend);
                    fading_text->alpha(alpha_max);
                    text->blend_mode(plai::BlendMode::Blend);
                    text->update(frm);
                    text->alpha(0);
                    mode = Blend;
                    break;
                }
                case Blend: {
                    std::println("blending: {}", alpha);
                    front->render_clear();
                    fading_text->alpha(alpha_max - alpha);
                    fading_text->render_to({});
                    text->alpha(alpha);
                    text->render_to({});
                    front->render_current();
                    if (alpha == alpha_max) {
                        mode = FinishBlend;
                    } else if (alpha > alpha_max - alpha_delta) {
                        alpha = alpha_max;
                    } else {
                        alpha += alpha_delta;
                    }
                    sleeper();
                    break;
                }
                case FinishBlend: {
                    std::println("finished blend: {}", alpha);
                    alpha = 0;
                    text->blend_mode(plai::BlendMode::None);
                    mode = Play;
                    break;
                };
                case Play: {
                    std::println("playing");
                    text->update(frm);
                    text->render_to({});
                    front->render_current();
                    sleeper();
                    ++frame_counter;
                    std::swap(prev_frm, frm);
                    ++iter;
                    std::println("frame counter: {}", frame_counter);
                    // remove hack, should only extract one frame from JPEG
                    // if (frame_counter < 10) {
                    // std::this_thread::sleep_for(3s); }
                    break;
                }
            }
        }
        if (frame_counter == 1) { std::this_thread::sleep_for(2s); }
    }
}
#endif
