#include <plai/logs/logs.hpp>
#include <plai/net/api.hpp>
#include <plai/net/http/exceptions.hpp>
#include <plai/net/http/server.hpp>
#include <plai/util/str.hpp>
#include <rfl/json.hpp>
#include <utility>

namespace plai::net {
namespace {

std::string to_str(const std::vector<std::string>& v) {
    std::string res = {"["};
    for (const auto& e : v) { res += '"' + e + "\","; }
    if (res.size() > 1)
        res.back() = ']';
    else
        res.push_back(']');
    return res;
}

}  // namespace
class ServerImpl final : public ApiServer {
 public:
    explicit ServerImpl(http::Server srv) noexcept : m_srv(std::move(srv)) {}

    void run() override { m_srv.run(); }
    void stop() override { m_srv.stop(); }

 private:
    http::Server m_srv;
};

namespace {
[[noreturn]] void not_implemented() {
    throw http::Exception({
        .status_code = PLAI_HTTP(501),
        .body = "not implemented",
    });
}

constexpr auto service_fn(auto&& fn) {
    return [fn = std::move(fn)](const http::Request& req) -> http::Response {
        try {
            return fn(req);
        } catch (const http::Exception& ex) {
            return {
                .body = ex.args().body,
                .status_code = static_cast<uint16_t>(ex.args().status_code),
            };
        }
    };
}
}  // namespace

auto DefaultV2Api::media_get(std::string_view key) -> std::optional<MediaMeta> {
    not_implemented();
}

auto DefaultV2Api::media_put(
    std::string_view key,
    std::function<std::optional<std::span<const uint8_t>>()> body)
    -> MediaPutStatus {
    not_implemented();
}

void DefaultV2Api::media_delete(std::string_view key) { not_implemented(); }

std::vector<std::string> DefaultV2Api::medias_get() { not_implemented(); }

void DefaultV2Api::medias_prune() { not_implemented(); }

auto DefaultV2Api::playlist_info(std::string_view key)
    -> std::optional<PlaylistInfo> {
    not_implemented();
}

void DefaultV2Api::playlist_activate(std::string_view key, bool active) {
    not_implemented();
}

void DefaultV2Api::playlist_medias_append(std::string_view key, Location loc,
                                          std::span<const std::string> medias) {
    not_implemented();
}

void DefaultV2Api::playlist_medias_delete(std::string_view key, Location loc,
                                          std::span<const std::string> medias) {
    not_implemented();
}

void DefaultV2Api::playlist_medias_rotate(std::string_view key, Location loc,
                                          std::span<const std::string> medias) {
    not_implemented();
}

void DefaultV2Api::playlist_set_image_duration(std::string_view key,
                                               size_t image_ms) {
    not_implemented();
}

std::unique_ptr<ApiServer> launch_api(ApiV2* api, std::string_view bind) {
    return std::make_unique<ServerImpl>(
        http::ServerBuilder()
            .bind(std::string(bind))
            .prefix("/plai/v2")
            .service("/_ping", http::METHOD_GET,
                     [api](const http::Request& req) -> http::Response {
                         api->ping();
                         return {
                             .body = "pong",
                             .status_code = PLAI_HTTP(200),
                         };
                     })
            .service(
                "/medias/items/{name}",
                http::METHOD_GET | http::METHOD_PUT | http::METHOD_DELETE,
                service_fn([api](const http::Request& req) -> http::Response {
                    auto name = req.target().path_params().at("name");
                    if (req.method() == http::METHOD_GET) {
                        auto meta = api->media_get(name);
                        if (!meta) {
                            return {
                                .body = "Media does not exist",
                                .status_code = PLAI_HTTP(404),
                            };
                        }
                        return {.body = plai::format(
                                    R"({{"digest":"sha256:{}","size":{}}})",
                                    crypto::hex_str(meta->digest), meta->size)};
                    }
                    if (req.method() == http::METHOD_PUT) {
                        api->media_put(name,
                                       [&]() { return req.data_chunked(); });
                        return {.body = "done", .status_code = PLAI_HTTP(200)};
                    }
                    if (req.method() == http::METHOD_DELETE) {
                        api->media_delete(name);
                    }
                    return {.body = "Unhandled HTTP method",
                            .status_code = PLAI_HTTP(500)};
                }))
            .service(
                "/medias", http::METHOD_GET,
                service_fn([api](const http::Request& req) -> http::Response {
                    (void)req;
                    auto res = api->medias_get();
                    return {.body = to_str(res), .status_code = PLAI_HTTP(200)};
                }))
            .service(
                "/playlists/items/{name}/medias",
                http::METHOD_PATCH | http::METHOD_PUT,
                service_fn([api](const http::Request& req) -> http::Response {
                    auto name = req.target().path_params().at("name");
                    if (req.method() == http::METHOD_PUT) {
                        auto medias = rfl::json::read<std::vector<std::string>>(
                            req.text());
                        if (!medias) {
                            return {
                                .body = "Invalid body",
                                .status_code = PLAI_HTTP(400),
                            };
                        }
                        PLAI_WARN(
                            "Setting playlist not supported, amending instead");
                        api->playlist_medias_append(name, ApiV2::Location::Next,
                                                    *medias);
                        return {
                            .body = "OK",
                            .status_code = PLAI_HTTP(200),
                        };
                    }
                    struct Body {
                        std::string action;
                        std::string location;
                        std::vector<std::string> items;
                    };
                    auto body = rfl::json::read<Body>(req.text());
                    if (!body) {
                        return {
                            .body = "Invalid body",
                            .status_code = PLAI_HTTP(400),
                        };
                    }
                    api->playlist_medias_append(name, ApiV2::Location::Next,
                                                (*body).items);
                    return {
                        .body = "OK",
                        .status_code = PLAI_HTTP(200),
                    };
                }))
            .service(
                "/playlists/items/{name}/active", http::METHOD_POST,
                service_fn([api](const http::Request& req) -> http::Response {
                    auto name = req.target().path_params().at("name");
                    auto body = rfl::json::read<bool>(req.text());
                    if (!body) {
                        return {
                            .body = "Invalid body",
                            .status_code = PLAI_HTTP(404),
                        };
                    }
                    api->playlist_activate(name, *body);
                    return {
                        .body = "OK",
                        .status_code = PLAI_HTTP(200),
                    };
                }))
            .commit());
}
}  // namespace plai::net
