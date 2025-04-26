#include <csignal>
#include <plai.hpp>
#include <print>

#include "cli.hpp"

namespace plaibin {

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
        std::format("{}/{}", plai::net::serialize_media_type(type), key);
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

 public:
    ApiImpl(plai::Store* store, Playlist* playlist)
        : Parent(store), m_playlist(playlist) {
        assert(playlist);
    }
    void play(const std::vector<plai::net::MediaListEntry>& medias,
              bool replay) override {
        m_playlist->set_entries(medias);
        m_playlist->set_repeat(replay);
        // TODO: indicate success/failure...
    }
};

int run(const Cli& args) {
    std::atomic<plai::play::Player*> ptr_player{};
    static constexpr std::array<plai::os::Signal, 1> mask{SIGINT};
    plai::os::SignalListener listener(mask, [&] {
        if (!ptr_player) return false;
        ptr_player.load()->stop();
        return true;
    });
    plai::logs::init(args.log_level);

    auto store = plai::sqlite_store(args.db);
    auto playlist = Playlist(store.get());
    auto api = ApiImpl(store.get(), &playlist);
    auto srv = plai::net::launch_api(&api, args.socket);
    auto srv_thread = std::jthread([&] { srv->run(); });
    auto ftype = args.void_frontend ? plai::FrontendType::Void
                                    : plai::FrontendType::Sdl2;
    auto frontend = plai::frontend(ftype);
    auto opts = plai::play::PlayerOpts{.wait_media = true};
    auto player = plai::play::Player(
        frontend.get(), &playlist, plai::play::PlayerOpts{.wait_media = true});
    ptr_player = &player;
    player.run();
    srv->stop();
    return 0;
}

int do_main(int argc, char** argv) {
    try {
        return run(parse_cli(argc, argv));
    } catch (const Exit& e) {
        return e.code();
    } catch (const std::exception& e) {
        std::println(stderr, "{}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

}  // namespace plaibin
int main(int argc, char** argv) { ::plaibin::do_main(argc, argv); }
