#include <plai/net/http/server.hpp>
#include <print>

int main() {
    plai::net::http::ServerBuilder builder{};
    builder.prefix("/plai/v1");
    std::string data{};
    builder
        .service("/store", plai::net::http::method::POST,
                 [&](const plai::net::http::Request& req,
                     plai::net::http::Response& resp) {
                     std::println("data: {}", req.body);
                     data = req.body;
                 })
        .service("/read", plai::net::http::method::GET,
                 [&](const plai::net::http::Request& req,
                     plai::net::http::Response& resp) {
                     resp.set_content(data, "text/plain");
                 })
        .run("0.0.0.0", 8080);
}
