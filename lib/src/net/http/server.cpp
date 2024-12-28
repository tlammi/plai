#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <filesystem>
#include <plai/logs/logs.hpp>
#include <plai/net/http/server.hpp>

namespace plai::net::http {
namespace local = boost::asio::local;
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace {

struct ServiceKey {
    std::string pattern;
    Method methods;

    constexpr bool operator<(const ServiceKey& other) const noexcept {
        if (pattern < other.pattern) return true;
        if (pattern > other.pattern) return false;
        if (methods < other.methods) return true;
        return false;
    }
};

using ServiceHandler = std::function<Response(const Request&)>;
using ServiceMap = std::map<ServiceKey, ServiceHandler>;

http::response<http::vector_body<uint8_t>> make_boost_response(
    const Response& resp, unsigned http_version) {
    http::response<http::vector_body<uint8_t>> out{
        http::int_to_status(resp.status_code), http_version};
    out.set(http::field::server, "plai media player");
    out.set(http::field::content_type, "text/plain");
    out.body() = std::move(resp.body);
    out.prepare_payload();
    return out;
}
}  // namespace

class Server::Impl {
 public:
    Impl(std::string socket, ServiceMap services)
        : m_sock(std::move(socket)), m_services(std::move(services)) {
        PLAI_DEBUG("registered API handlers:");
        for (const auto& [key, _] : m_services) {
            PLAI_DEBUG("  '{}'", key.pattern);
        }
    }

    void run() {
        if (fs::exists(m_sock)) { fs::remove(m_sock); }
        auto ioc = boost::asio::io_context();
        auto acceptor = local::stream_protocol::acceptor(
            ioc, local::stream_protocol::endpoint(m_sock));
        while (true) {
            auto sock = local::stream_protocol::socket(ioc);
            acceptor.accept(sock);
            auto stream =
                beast::basic_stream<local::stream_protocol>(std::move(sock));
            beast::flat_buffer buf{};
            http::request<http::vector_body<uint8_t>> req{};
            http::read(stream, buf, req);
            auto target_str = std::string_view(req.target());
            for (const auto& [key, handler] : m_services) {
                auto tgt = parse_target(key.pattern, target_str);
                if (tgt) {
                    auto resp = handler(
                        Request{.target = *std::move(tgt), .body = req.body()});
                    auto boost_resp = make_boost_response(resp, req.version());
                    /*
                    auto boost_resp =
                        http::response<http::vector_body<uint8_t>>(
                            http::status::ok, req.version());
                    boost_resp.set(http::field::server, "Plai media player");
                    boost_resp.set(http::field::content_type, "text/plain");
                    boost_resp.prepare_payload();
                    */
                    http::write(stream, boost_resp);
                }
            }
        }
    }

 private:
    http::response<http::vector_body<uint8_t>> handle_request(
        http::request<http::vector_body<uint8_t>>& req) {
        auto target_str = std::string_view(req.target());
        for (const auto& [key, handler] : m_services) {
            auto tgt = parse_target(key.pattern, target_str);
            if (tgt) {
                auto resp = handler(
                    Request{.target = std::move(*tgt), .body = req.body()});
                auto boost_resp = http::response<http::vector_body<uint8_t>>(
                    http::status::ok, req.version());
                boost_resp.set(http::field::server, "Plai media player");
                boost_resp.set(http::field::content_type, "text/plain");
                boost_resp.prepare_payload();
                return boost_resp;
            }
        }
        auto resp = http::response<http::vector_body<uint8_t>>(
            http::status::not_found, req.version());
        resp.set(http::field::server, "Plai media player");
        resp.set(http::field::content_type, "text/plain");
        resp.prepare_payload();
        return resp;
    }
    fs::path m_sock;
    ServiceMap m_services;
};

Server::Server(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}
Server::~Server() {}

void Server::run() { m_impl->run(); }

class ServerBuilder::Impl {
 public:
    Impl() = default;

    void prefix(std::string prefix) { m_prefix = std::move(prefix); }

    void bind(std::string socket) { m_sock = std::move(socket); }

    void service(std::string pattern, Method methods,
                 std::function<Response(const Request&)> handler) {
        m_services[{std::move(pattern), methods}] = std::move(handler);
    }

    Server commit() {
        ServiceMap processed{};
        for (auto&& [key, val] : m_services) {
            processed[{m_prefix + key.pattern, key.methods}] = std::move(val);
        }
        return {std::make_unique<Server::Impl>(std::move(m_sock),
                                               std::move(processed))};
    }

 private:
    std::string m_prefix{};
    std::string m_sock{};
    ServiceMap m_services{};
};

ServerBuilder::ServerBuilder() : m_impl(std::make_unique<Impl>()) {}
ServerBuilder::~ServerBuilder() {}

auto ServerBuilder::prefix(std::string prefix) -> Self& {
    m_impl->prefix(std::move(prefix));
    return *this;
}

auto ServerBuilder::bind(std::string socket) -> Self& {
    m_impl->bind(std::move(socket));
    return *this;
}
auto ServerBuilder::service(std::string pattern, Method methods,
                            std::function<Response(const Request&)> handler)
    -> Self& {
    m_impl->service(std::move(pattern), methods, std::move(handler));
    return *this;
}

Server ServerBuilder::commit() { return m_impl->commit(); }
}  // namespace plai::net::http
