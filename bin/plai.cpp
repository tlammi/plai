#include <csignal>
#include <mutex>
#include <plai.hpp>
#include <plai/ctx/ctx.hpp>
#include <upp/linux/signal.hpp>

#include "cli.hpp"

namespace plaibin {
using namespace std::literals;

plai::media::Media read_media(plai::Store& store, const std::string& entry) {
    return plai::media::Media(store.read(entry));
}

class Playlist final : public plai::play::MediaSrc {
    std::mutex m_mut{};
    plai::Store* m_store;
    std::vector<std::string> m_keys{};
    size_t m_idx{0};
    bool m_repeat{true};

 public:
    Playlist(plai::Store* store) : m_store(store) { assert(store); }

    bool set_entries(std::vector<std::string> entries) {
        auto lk = std::lock_guard(m_mut);
        m_keys = std::move(entries);
        // TODO: Check that entries actually exist and lock them
        m_idx = 0;
        return true;
    }
    void set_repeat(bool val) {
        auto lk = std::lock_guard(m_mut);
        m_repeat = val;
    }

    std::optional<plai::media::Media> next_media() override {
        auto lk = std::lock_guard(m_mut);
        if (m_keys.empty()) return std::nullopt;
        if (m_idx >= m_keys.size()) {
            if (!m_repeat) return std::nullopt;
            m_idx = 0;
        }
        return read_media(*m_store, m_keys.at(std::exchange(m_idx, m_idx + 1)));
    }
};

int run(const Cli& args) {
    std::atomic<plai::play::Player*> ptr_player{};
    auto listener = upp::linux::signal_listener{SIGINT};
    auto waiter = std::jthread([&] {
        while (true) {
            auto sig = listener.wait();
            PLAI_INFO("Received signal");
            (void)sig;
            if (ptr_player) {
                PLAI_DEBUG("Notifying player");
                ptr_player.load()->stop();
                return;
            }
            PLAI_WARN("Player has not been registered yet. Ignoring signal.");
        }
    });
    plai::logs::init(args.log_level, args.log_file);

    auto store = plai::sqlite_store(args.db);
    auto playlist = Playlist(store.get());
    auto ftype = args.void_frontend ? plai::FrontendType::Void
                                    : plai::FrontendType::Sdl2;
    auto frontend = plai::frontend(ftype);
    frontend->set_fullscreen(args.fullscreen);
    auto opts = plai::play::PlayerOpts{
        .accel = std::move(args.accel),
        .image_dur = args.img_dur,
        .blend_dur = args.blend,
        .wait_media = true,
    };

    if (!args.background.empty()) {
        PLAI_INFO("using background from {}", args.background);
        auto frm = plai::media::decode_image(args.background);
        opts.background = {
            .image = std::move(frm),
            .target = args.background_tgt,
        };
    }

    if (!args.watermark.empty()) {
        PLAI_INFO("using watermark from {}", args.watermark);
        auto frm = plai::media::decode_image(args.watermark);
        opts.watermarks.push_back(
            {.image = std::move(frm), .target = args.watermark_tgt});
    }

    auto ctx = plai::ctx::make_context();
    auto player =
        plai::play::Player(frontend.get(), ctx.get(), std::move(opts));
    // auto api = ApiImpl(store.get(), &playlist, &player);
    auto srv = plai::net::launch_api(ctx.get(), args.socket);
    auto srv_thread = std::jthread([&] { srv->run(); });
    ptr_player = &player;
    player.run();
    PLAI_INFO("Player exited");
    srv->stop();
    return 0;
}

int do_main(int argc, char** argv) {
    try {
        auto args = parse_cli(argc, argv);
        if (args.list_accel) {
            for (auto accel : plai::media::supported_hardware_accelerators()) {
                plai::println("{}", accel.name());
            }
            return EXIT_SUCCESS;
        }
        return run(args);
    } catch (const Exit& e) {
        return e.code();
    } catch (const std::exception& e) {
        plai::println(stderr, "{}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

}  // namespace plaibin
int main(int argc, char** argv) { ::plaibin::do_main(argc, argv); }
