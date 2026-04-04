#include <plai/logs/logs.hpp>
#include <plai/net/api.hpp>
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

MediaMeta DefaultApi::media_get(std::string_view key) {
    auto s = std::string(key);
    auto res = m_store->inspect(s);
    return {
        .size = res->bytes,
        .digest = res->sha256,
    };
}

void DefaultApi::media_put(
    std::string_view key,
    std::function<std::optional<std::span<const uint8_t>>()> body) {
    std::vector<uint8_t> buf{};
    while (true) {
        auto r = body();
        if (!r) break;
        buf.insert(buf.end(), r->begin(), r->end());
    }
    auto s = std::string(key);
    PLAI_INFO("media {} with size {}", s, buf.size());
    m_store->store(s, buf);
}

DeleteResult DefaultApi::media_delete(std::string_view key) {
    auto s = std::string(key);
    PLAI_INFO("deleting media {}", s);
    // TODO: cannot get info whether was deleted. Do something
    m_store->remove(s);
    PLAI_WARN("checking remove result has not been implemented!!");
    return DeleteResult::Success;
}

std::vector<std::string> DefaultApi::get_medias() {
    PLAI_TRACE("listing medias");
    auto entries = m_store->list();
    std::vector<std::string> out{};
    out.reserve(entries.size());
    for (const auto& e : entries) { out.emplace_back(e); }
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
                "/medias/items/{name}",
                http::METHOD_GET | http::METHOD_PUT | http::METHOD_DELETE,
                [api](const http::Request& req) -> http::Response {
                    auto name = req.target().path_params().at("name");
                    if (req.method() == http::METHOD_GET) {
                        auto meta = api->media_get(name);
                        return {.body = plai::format(
                                    R"({{"digest":"sha256:{}","size":{}}})",
                                    crypto::hex_str(meta.digest), meta.size)};
                    }
                    if (req.method() == http::METHOD_PUT) {
                        api->media_put(name,
                                       [&]() { return req.data_chunked(); });
                        return {.body = "done", .status_code = PLAI_HTTP(200)};
                    }
                    if (req.method() == http::METHOD_DELETE) {
                        auto res = api->media_delete(name);

                        using enum net::DeleteResult;
                        switch (res) {
                            case DeleteResult::Success:
                                return {.body = "done",
                                        .status_code = PLAI_HTTP(200)};
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
                         auto res = api->get_medias();
                         return {.body = to_str(res),
                                 .status_code = PLAI_HTTP(200)};
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
                         bool replay = true;
                         const auto& query_params = req.query_params();
                         if (query_params.contains("replay")) {
                             std::string_view replay_str =
                                 query_params.at("replay");
                             if (replay_str == "true") {
                                 replay = true;
                             } else if (replay_str == "false") {
                                 replay = false;
                             } else {
                                 return {.body = "Invalid replay parameter",
                                         .status_code = PLAI_HTTP(400)};
                             }
                         }
                         api->play(*parsed, replay);
                         return {.body = "OK", .status_code = PLAI_HTTP(200)};
                     })
            .commit());
}

}  // namespace plai::net
