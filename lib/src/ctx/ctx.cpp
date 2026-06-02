#include <map>
#include <mutex>
#include <plai/ctx/ctx.hpp>
#include <plai/ctx/multi_playlist_cursor.hpp>
#include <plai/logs/logs.hpp>
#include <plai/net/http/exceptions.hpp>
#include <plai/net/http/macros.hpp>

namespace plai::ctx {
namespace {

struct MediaData {
    net::MediaMeta meta{};
    media::Media data{};
};
std::string body_from_status(unsigned status_code) {
    // NOLINTBEGIN(*magic*)
    switch (status_code) {
        case 404: return "not found";
        case 501: return "not implemented";
    }
    return "";
    // NOLINTEND(*magic*)
}
[[noreturn]] void throw_http(unsigned status_code) {
    throw net::http::Exception(
        {.status_code = status_code, .body = body_from_status(status_code)});
}

MediaData make_media(std::vector<uint8_t> data) {
    return {
        .meta =
            {
                .size = data.size(),
                .digest = crypto::sha256(data),
            },
        .data = media::Media(std::move(data)),
    };
}
}  // namespace

class CtxImpl final : public Ctx {
    [[nodiscard]] auto do_lock() { return std::unique_lock(m_mut); }

 public:
    std::optional<net::MediaMeta> media_get(std::string_view key) override {
        auto lk = do_lock();
        auto it = m_medias.find(key);
        if (it == m_medias.end()) throw_http(PLAI_HTTP(404));
        return it->second.meta;
    }

    MediaPutStatus media_put(
        std::string_view key,
        std::function<std::optional<std::span<const uint8_t>>()> body)
        override {
        auto data = std::vector<uint8_t>();
        for (auto chunk = body(); chunk; chunk = body()) {
            data.append_range(*chunk);
        }
        auto lk = do_lock();
        auto it = m_medias.find(key);
        if (it == m_medias.end()) {
            m_medias[std::string(key)] = make_media(std::move(data));
            return MediaPutStatus::Created;
        }
        it->second = make_media(std::move(data));
        return MediaPutStatus::Ok;
    }

    void media_delete(std::string_view key) override {
        auto lk = do_lock();
        // TODO: Check that not part of any playlist
        auto it = m_medias.find(key);
        if (it == m_medias.end()) throw_http(PLAI_HTTP(404));
        m_medias.erase(it);
    }

    std::vector<std::string> medias_get() override {
        auto lk = do_lock();
        return m_medias | std::views::keys | std::ranges::to<std::vector>();
    }

    void medias_prune() override {
        auto lk = do_lock();
        throw_http(PLAI_HTTP(501));
    }

    std::optional<PlaylistInfo> playlist_info(std::string_view key) override {
        auto lk = do_lock();
        using namespace std::chrono;
        // TODO: Check if exists
        if (!m_cursor.contains(key)) return std::nullopt;
        auto& pl = m_cursor[key];
        auto winsize = pl.cursor.window_size();
        return PlaylistInfo{
            .medias = {},
            .window_size = winsize == PlaylistCursor::npos ? 0 : winsize,
            .image_ms = static_cast<size_t>(
                duration_cast<seconds>(pl.image_duration).count()),
            .active = pl.active,
        };
    }

    void playlist_activate(std::string_view key, bool active) override {
        auto lk = do_lock();
        m_cursor[key].active = active;
    }

    void playlist_medias_append(std::string_view key, Location loc,
                                std::span<const std::string> medias) override {
        auto lk = do_lock();
        using enum Location;
        switch (loc) {
            case Front: m_cursor[key].cursor.insert_front(medias); break;
            case Back: m_cursor[key].cursor.insert_back(medias); break;
            case Next: m_cursor[key].cursor.insert_next(medias); break;
        }
    }

    void playlist_medias_delete(std::string_view key, Location loc,
                                std::span<const std::string> medias) override {
        auto lk = do_lock();
        throw_http(PLAI_HTTP(501));
    }

    void playlist_medias_rotate(std::string_view key, Location loc,
                                std::span<const std::string> medias) override {
        auto lk = do_lock();
        auto& cursor = m_cursor[key].cursor;
        auto orig_size = cursor.entries().size();
        using enum Location;
        switch (loc) {
            case Front: cursor.insert_front(medias); break;
            case Back: cursor.insert_back(medias); break;
            case Next: cursor.insert_next(medias); break;
        }
        cursor.trim_to_size(orig_size);
    }

    void playlist_set_image_duration(std::string_view key,
                                     size_t image_ms) override {
        auto lk = do_lock();
        m_cursor[key].image_duration = std::chrono::milliseconds(image_ms);
    }

    std::optional<media::Media> next_media() override {
        auto lk = do_lock();
        if (!m_curr) {
            m_cursor.reset();
            m_curr = m_cursor.next();
        }
        while (m_curr) {
            auto res = m_curr->cursor.next();
            if (!res.empty()) return m_medias.at(res).data;
            m_curr = m_cursor.next();
        }
        return std::nullopt;
    }

 private:
    std::mutex m_mut{};
    std::map<std::string, MediaData, std::less<>> m_medias{};
    MultiPlaylistCursor m_cursor{};
    MultiPlaylistCursor::Playlist* m_curr{};
};

std::unique_ptr<Ctx> make_context() { return std::make_unique<CtxImpl>(); }
}  // namespace plai::ctx
