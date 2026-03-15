#include <exec/single_thread_context.hpp>
#include <plai/mods/player.hpp>

class Playlist final : public plai::mods::MediaStream {
    plai::ex::AnySenderOf<plai::media2::Media> next_media() override {
        return stdexec::just(plai::media2::Media({}));
    }
};

struct Ctx final : public plai::mods::Context {
    stdexec::run_loop loop{};

    plai::ex::AnyScheduler scheduler() override { return loop.get_scheduler(); }

    void request_finish() override { loop.finish(); }
};

int main() {
    auto playlist = Playlist();
    auto player = plai::mods::Player(playlist);

    auto ctx = Ctx();
    player.start(ctx);
    ctx.loop.run();
}
