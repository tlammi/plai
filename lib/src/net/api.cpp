#include <plai/net/api.hpp>
#include <plai/net/http/server.hpp>
#include <utility>

namespace plai::net {
class ServerImpl final : public ApiServer {
 public:
    constexpr explicit ServerImpl(http::Server srv) noexcept
        : m_srv(std::move(srv)) {}

    void run() override { m_srv.run(); }

 private:
    http::Server m_srv;
};

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
                "/media/{type}/{name}", http::METHOD_PUT | http::METHOD_DELETE,
                [api](const http::Request& req) -> http::Response {
                    auto type =
                        parse_media_type(req.target().params().at("type"));
                    if (!type) {
                        return {.body = "invalid media type",
                                .status_code = PLAI_HTTP(400)};
                    }
                    auto name = req.target().params().at("name");
                    if (req.method() == http::METHOD_PUT) {
                        api->put_media(*type, name,
                                       [&]() { return req.data_chunked(); });
                        return {.body = "done", .status_code = PLAI_HTTP(200)};
                    } else if (req.method() == http::METHOD_DELETE) {
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
            .commit());
}

}  // namespace plai::net
