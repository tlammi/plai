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
            .commit());
}

}  // namespace plai::net
