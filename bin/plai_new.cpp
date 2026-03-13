#include <exec/single_thread_context.hpp>
#include <plai/mods/player.hpp>

class Playlist final : public plai::mods::MediaStream {
    plai::ex::AnySenderOf<plai::media2::Media> next_media() override {
        return stdexec::just(plai::media2::Media({}));
    }
};

struct Ctx final : public plai::mods::Context {
    plai::ex::AnyScheduler sched;

    explicit Ctx(plai::ex::AnyScheduler sched) : sched(std::move(sched)) {}

    plai::ex::AnyScheduler scheduler() override { return sched; }

    void request_stop() override {}
};

int main() {
    auto playlist = Playlist();
    auto player = plai::mods::Player(playlist);

    auto loop = exec::single_thread_context();

    auto ctx = Ctx(loop.get_scheduler());
    player.start(ctx);
}
