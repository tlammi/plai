#include <plai/logs/logs.hpp>
#include <plai/net/api.hpp>
#include <plai/net/http/server.hpp>
#include <plai/util/str.hpp>
#include <rfl/json.hpp>
#include <utility>

namespace plai::net {
namespace {
std::string to_str(const MediaListEntry& v) {
    return std::format(R"({{"type": "{}", "key": "{}"}})",
                       serialize_media_type(v.type), v.key);
}
std::string to_str(const std::vector<MediaListEntry>& v) {
    std::string res = {"["};
    for (const auto& e : v) { res += to_str(e) + ","; }
    if (res.size() > 1)
        res.back() = ']';
    else
        res.push_back(']');
    return res;
}

std::optional<MediaListEntry> parse_media_list_entry(std::string_view value) {
    auto [type_str, key] = split_left(value, "/");
    auto type = parse_media_type(type_str);
    if (!type) return std::nullopt;
    return MediaListEntry{*type, std::string(key)};
}

std::optional<std::vector<MediaListEntry>> parse_media_list(
    std::span<const std::string> media_list) {
    auto out = std::vector<MediaListEntry>();
    out.reserve(media_list.size());
    for (const auto& str : media_list) {
        auto entry = parse_media_list_entry(str);
        if (!entry) return std::nullopt;
        out.push_back(std::move(*entry));
    }
    return out;
}
}  // namespace
class ServerImpl final : public ApiServer {
 public:
    constexpr explicit ServerImpl(http::Server srv) noexcept
        : m_srv(std::move(srv)) {}

    void run() override { m_srv.run(); }

 private:
    http::Server m_srv;
};

MediaMeta DefaultApi::get_media(MediaType type, std::string_view key) {
    auto s = std::format("{}/{}", plai::net::serialize_media_type(type), key);
    auto res = m_store->inspect(s);
    return {
        .size = res->bytes,
        .digest = res->sha256,
    };
}

void DefaultApi::put_media(
    MediaType type, std::string_view key,
    std::function<std::optional<std::span<const uint8_t>>()> body) {
    std::vector<uint8_t> buf{};
    while (true) {
        auto r = body();
        if (!r) break;
        buf.insert(buf.end(), r->begin(), r->end());
    }
    auto s = std::format("{}/{}", plai::net::serialize_media_type(type), key);
    PLAI_INFO("media {} with size {}", s, buf.size());
    m_store->store(s, buf);
}

DeleteResult DefaultApi::delete_media(MediaType type, std::string_view key) {
    auto s = std::format("{}/{}", plai::net::serialize_media_type(type), key);
    PLAI_INFO("deleting media {}", s);
    // TODO: cannot get info whether was deleted. Do something
    m_store->remove(s);
    PLAI_WARN("checking remove result has not been implemented!!");
    return DeleteResult::Success;
}

std::vector<MediaListEntry> DefaultApi::get_medias(
    std::optional<MediaType> type) {
    PLAI_TRACE("listing medias");
    auto entries = m_store->list();
    std::vector<MediaListEntry> out{};
    out.reserve(entries.size());
    for (const auto& e : entries) {
        auto [type_name, key] = plai::split_left(e, "/");
        auto type_v = plai::net::parse_media_type(type_name);
        if (!type_v)
            throw std::runtime_error("Corrupted database: Invalid key prefix");
        out.emplace_back(*type_v, std::string(key));
    }
    return out;
}

std::unique_ptr<ApiServer> launch_api(ApiV1* api, std::string_view bind) {
    return std::make_unique<ServerImpl>(
        http::ServerBuilder()
            .bind(std::string(bind))
            .prefix("/plai/v1")
            .service("/_ping", http::METHOD_GET,
                     [api](const http::Request& req) -> http::Response {
                         api->ping();
                         return {
                             .body = "pong",
                             .status_code = PLAI_HTTP(200),
                         };
                     })
            .service(
                "/media/{type}/{name}",
                http::METHOD_GET | http::METHOD_PUT | http::METHOD_DELETE,
                [api](const http::Request& req) -> http::Response {
                    auto type =
                        parse_media_type(req.target().path_params().at("type"));
                    if (!type) {
                        return {.body = "invalid media type",
                                .status_code = PLAI_HTTP(400)};
                    }
                    auto name = req.target().path_params().at("name");
                    if (req.method() == http::METHOD_GET) {
                        auto meta = api->get_media(*type, name);
                        return {.body = std::format(
                                    R"({{"digest":"sha256:{}","size":{}}})",
                                    crypto::hex_str(meta.digest), meta.size)};
                    }
                    if (req.method() == http::METHOD_PUT) {
                        api->put_media(*type, name,
                                       [&]() { return req.data_chunked(); });
                        return {.body = "done", .status_code = PLAI_HTTP(200)};
                    }
                    if (req.method() == http::METHOD_DELETE) {
                        auto res = api->delete_media(*type, name);

                        using enum net::DeleteResult;
                        switch (res) {
                            case DeleteResult::Success:
                                return {.body = "done",
                                        .status_code = PLAI_HTTP(200)};
                            case DeleteResult::Scheduled:
                                return {.body = "scheduled",
                                        .status_code = PLAI_HTTP(201)};
                            case DeleteResult::Failure:
                                return {.body = "Not found",
                                        .status_code = PLAI_HTTP(404)};
                        }
                        std::unreachable();
                    }
                    return {.body = "Unhandled HTTP method",
                            .status_code = PLAI_HTTP(500)};
                })
            .service("/media", http::METHOD_GET,
                     [api](const http::Request& req) -> http::Response {
                         (void)req;
                         auto res = api->get_medias(std::nullopt);
                         return {.body = to_str(res),
                                 .status_code = PLAI_HTTP(200)};
                     })
            .service(
                "/media/{type}", http::METHOD_GET,
                [api](const http::Request& req) -> http::Response {
                    auto type =
                        parse_media_type(req.target().path_params().at("type"));
                    if (!type)
                        return {.body = "invalid media type",
                                .status_code = PLAI_HTTP(400)};
                    auto res = api->get_medias(type);
                    return {.body = to_str(res), .status_code = PLAI_HTTP(200)};
                })
            .service("/play", http::METHOD_POST,
                     [api](const http::Request& req) -> http::Response {
                         auto txt = req.text();
                         PLAI_DEBUG("got playlist: {}", txt);
                         auto parsed =
                             rfl::json::read<std::vector<std::string>>(txt);
                         if (!parsed) {
                             return {.body = "Invalid playlist",
                                     .status_code = PLAI_HTTP(400)};
                         }
                         auto list = parse_media_list(*parsed);
                         if (!list) {
                             return {.body = "Invalid playlist entry",
                                     .status_code = PLAI_HTTP(400)};
                         }
                         // TODO: Support query parameters
                         api->play(*list, true);
                         return {.body = "OK", .status_code = PLAI_HTTP(200)};
                     })
            .commit());
}

}  // namespace plai::net
