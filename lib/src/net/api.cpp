#include <plai/net/api.hpp>
#include <plai/net/http/server.hpp>

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
            .service("/media/{type}/{name}", http::METHOD_PUT,
                     [api](const http::Request& req) -> http::Response {
                         auto type =
                             parse_media_type(req.target.params().at("type"));
                         if (!type) {
                             return {.body = "invalid media type",
                                     .status_code = PLAI_HTTP(400)};
                         }
                         auto name = req.target.params().at("name");
                         api->put_media(*type, name, {});
                         return {.body = "done", .status_code = PLAI_HTTP(200)};
                     })
            .commit());
}

}  // namespace plai::net
