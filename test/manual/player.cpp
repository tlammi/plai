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
