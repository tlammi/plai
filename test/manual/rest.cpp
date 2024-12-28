#include <plai/net/http/server.hpp>
#include <print>

using plai::net::http::Request;
using plai::net::http::Response;
int main(int argc, char** argv) {
    if (argc != 2) { throw plai::ValueError("usage: rest [socket]"); }
    auto builder = plai::net::http::server();
    builder.prefix("/plai/v1");
    std::string data{};
    auto srv =
        builder
            .service("/store", plai::net::http::METHOD_POST,
                     [&](const Request& req) -> Response {
                         data = std::string(req.body.begin(), req.body.end());
                         std::println("data: {}", data);
                         return {};
                     })
            .service("/read", plai::net::http::METHOD_GET,
                     [&](const Request& req) -> Response {
                         auto v = std::vector<uint8_t>();
                         v.reserve(data.size());
                         for (char c : data) { v.push_back(c); }
                         return {std::move(v)};
                     })
            .bind(argv[1])
            .commit();
    srv.run();
}
