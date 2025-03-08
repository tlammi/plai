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

constexpr size_t CHUNK_SIZE = 10 * 1024 * 1024;

constexpr std::optional<Method> convert_boost_verb(http::verb v) {
    switch (v) {
        case boost::beast::http::verb::get: return METHOD_GET;
        case boost::beast::http::verb::delete_: return METHOD_DELETE;
        case boost::beast::http::verb::post: return METHOD_POST;
        case boost::beast::http::verb::put: return METHOD_PUT;
        case boost::beast::http::verb::unknown:
        case boost::beast::http::verb::head:
        case boost::beast::http::verb::connect:
        case boost::beast::http::verb::options:
        case boost::beast::http::verb::trace:
        case boost::beast::http::verb::copy:
        case boost::beast::http::verb::lock:
        case boost::beast::http::verb::mkcol:
        case boost::beast::http::verb::move:
        case boost::beast::http::verb::propfind:
        case boost::beast::http::verb::proppatch:
        case boost::beast::http::verb::search:
        case boost::beast::http::verb::unlock:
        case boost::beast::http::verb::bind:
        case boost::beast::http::verb::rebind:
        case boost::beast::http::verb::unbind:
        case boost::beast::http::verb::acl:
        case boost::beast::http::verb::report:
        case boost::beast::http::verb::mkactivity:
        case boost::beast::http::verb::checkout:
        case boost::beast::http::verb::merge:
        case boost::beast::http::verb::msearch:
        case boost::beast::http::verb::notify:
        case boost::beast::http::verb::subscribe:
        case boost::beast::http::verb::unsubscribe:
        case boost::beast::http::verb::patch:
        case boost::beast::http::verb::purge:
        case boost::beast::http::verb::mkcalendar:
        case boost::beast::http::verb::link:
        case boost::beast::http::verb::unlink: break;
    }
    return std::nullopt;
}

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

http::response<http::string_body> make_boost_response(const Response& resp,
                                                      unsigned http_version) {
    http::response<http::string_body> out{http::int_to_status(resp.status_code),
                                          http_version};
    out.set(http::field::server, "plai media player");
    out.set(http::field::content_type, "text/plain");
    out.body() = std::move(resp.body);
    out.prepare_payload();
    return out;
}
}  // namespace

// wrapper for beast parser, allows lazy reading
class RequestImpl final : public Request {
    Target m_target{};
    std::string_view m_body{};

 public:
    Target& target() noexcept { return m_target; }
    std::string_view& text() noexcept { return m_body; }

    const Target& target() const noexcept override { return m_target; }
    std::string_view text() const noexcept override { return m_body; }
};

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
        while (true) { step(ioc, acceptor); }
    }

 private:
    void step(boost::asio::io_context& ioc,
              local::stream_protocol::acceptor& acceptor) {
        auto sock = local::stream_protocol::socket(ioc);
        acceptor.accept(sock);
        auto stream =
            beast::basic_stream<local::stream_protocol>(std::move(sock));
        http::request_parser<http::buffer_body> parser{};
        parser.body_limit(std::numeric_limits<uint64_t>::max());

        beast::flat_buffer buf{};
        http::read_header(stream, buf, parser);
        auto tgt_str = std::string(parser.get().target());
        auto verb = convert_boost_verb(parser.get().method());
        auto ver = parser.get().version();
        PLAI_DEBUG(
            "transfer encoding: {}",
            std::string_view(parser.get()[http::field::transfer_encoding]));
        if (!verb) {
            http::write(stream, make_boost_response(
                                    Response{.body = "Unsupported method",
                                             .status_code = PLAI_HTTP(405)},
                                    ver));
            return;
        }
        for (const auto& [key, handler] : m_services) {
            if (!(key.methods & *verb)) continue;
            auto tgt = parse_target(key.pattern, tgt_str);
            if (tgt) {
                PLAI_DEBUG("handler: '{}'", key.pattern);
                boost::system::error_code ec{};
                std::vector<uint8_t> body{};
                auto body_buf =
                    std::make_unique<std::array<uint8_t, CHUNK_SIZE>>();
                while (!parser.is_done()) {
                    parser.get().body().data = body_buf->data();
                    parser.get().body().size = body_buf->size();
                    http::read(stream, buf, parser, ec);
                    if (ec && ec != http::error::need_buffer) {
                        throw boost::system::system_error(ec);
                    }
                    PLAI_DEBUG("got chunk with size: {}",
                               parser.get().body().size);
                    body.insert(body.end(), body_buf->begin(),
                                body_buf->begin() + parser.get().body().size);
                }
                auto r = RequestImpl();
                r.target() = *std::move(tgt);
                r.text() = {reinterpret_cast<const char*>(body.data()),
                            body.size()};
                auto resp = handler(r);
                auto boost_resp = make_boost_response(resp, ver);
                http::write(stream, boost_resp);
                return;
            }
        }
        http::write(stream,
                    make_boost_response(Response{.body = "Not found",
                                                 .status_code = PLAI_HTTP(404)},
                                        ver));
    }

    fs::path m_sock;
    ServiceMap m_services;
};

Server::Server(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

Server::Server(Server&&) noexcept = default;
Server& Server::operator=(Server&&) noexcept = default;
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
