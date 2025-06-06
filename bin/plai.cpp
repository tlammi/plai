#include <csignal>
#include <plai.hpp>

#include "cli.hpp"

namespace plaibin {
using namespace std::literals;

plai::media::Media mk_media(plai::net::MediaType t, auto data) {
    using enum plai::net::MediaType;
    switch (t) {
        case Video: return plai::media::Video(std::move(data));
        case Image: return plai::media::Image(std::move(data));
    }
    std::unreachable();
}

plai::media::Media read_media(plai::Store& store,
                              const plai::net::MediaListEntry& entry) {
    const auto& [type, key] = entry;
    auto full_key =
        plai::format("{}/{}", plai::net::serialize_media_type(type), key);
    return mk_media(type, store.read(full_key));
}

class Playlist final : public plai::play::MediaSrc {
    std::mutex m_mut{};
    plai::Store* m_store;
    std::vector<plai::net::MediaListEntry> m_keys{};
    size_t m_idx{0};
    bool m_repeat{true};

 public:
    Playlist(plai::Store* store) : m_store(store) { assert(store); }

    bool set_entries(std::vector<plai::net::MediaListEntry> entries) {
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

class ApiImpl : public plai::net::DefaultApi {
    using Parent = plai::net::DefaultApi;

    Playlist* m_playlist;
    plai::play::Player* m_player;

 public:
    ApiImpl(plai::Store* store, Playlist* playlist, plai::play::Player* player)
        : Parent(store), m_playlist(playlist), m_player(player) {
        assert(playlist);
    }
    void play(const std::vector<plai::net::MediaListEntry>& medias,
              bool replay) override {
        m_playlist->set_entries(medias);
        m_playlist->set_repeat(replay);
        m_player->clear_media_queue();
        // TODO: indicate success/failure...
    }
};

int run(const Cli& args) {
    auto start = plai::Clock::now();
    static constexpr auto player_timeout = 5s;
    std::atomic<plai::play::Player*> ptr_player{};
    static constexpr std::array<plai::os::Signal, 1> mask{SIGINT};
    plai::os::SignalListener listener(mask, [&] {
        if (ptr_player) {
            ptr_player.load()->stop();
            return true;
        }
        return plai::Clock::now() - start > player_timeout;
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

    if (!args.watermark.empty()) {
        PLAI_INFO("using watermark from {}", args.watermark);
        auto frm = plai::media::decode_image(args.watermark);
        opts.watermarks.push_back(
            {.image = std::move(frm), .target = args.watermark_tgt});
    }
    auto player =
        plai::play::Player(frontend.get(), &playlist, std::move(opts));
    auto api = ApiImpl(store.get(), &playlist, &player);
    auto srv = plai::net::launch_api(&api, args.socket);
    auto srv_thread = std::jthread([&] { srv->run(); });
    ptr_player = &player;
    player.run();
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
