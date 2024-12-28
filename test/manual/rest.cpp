#include <plai/net/http/server.hpp>
#include <print>

using plai::net::http::Request;
using plai::net::http::Response;
int main(int argc, char** argv) {
    if (argc != 2) { throw plai::ValueError("usage: rest [socket]"); }
    auto builder = plai::net::http::server();
    builder.prefix("/plai/v1");
    std::map<std::string, std::string> dict{};
    auto srv =
        builder
            .service("/store/{key}", plai::net::http::METHOD_POST,
                     [&](const Request& req) -> Response {
                         std::println("data: {}", req.body);
                         dict[std::string(req.target.at("key"))] =
                             std::string(req.body);
                         return {};
                     })
            .service("/read/{key}", plai::net::http::METHOD_GET,
                     [&](const Request& req) -> Response {
                         return {dict[std::string(req.target.at("key"))]};
                     })
            .bind(argv[1])
            .commit();
    srv.run();
}
