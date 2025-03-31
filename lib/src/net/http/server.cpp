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

struct ParsingCtx {
    static constexpr size_t CHUNK_SIZE = 10 * 1024 * 1024;
    beast::flat_buffer flat_buf{};
    beast::basic_stream<local::stream_protocol> stream;
    http::request_parser<http::buffer_body> parser{};
    std::vector<uint8_t> buf{};
    unsigned http_version{};

    explicit ParsingCtx(
        beast::basic_stream<local::stream_protocol> stream) noexcept
        : stream(std::move(stream)) {
        parser.body_limit(std::numeric_limits<size_t>::max());
    }

    void read_header() {
        http::read_header(stream, flat_buf, parser);
        http_version = parser.get().version();
    }

    bool is_done() const noexcept { return parser.is_done(); }

    boost::system::error_code read_next() noexcept {
        // append next chunk to buf
        auto offset = buf.size();
        buf.resize(buf.size() + CHUNK_SIZE);
        auto& body = parser.get().body();
        body.data = &buf.at(offset);
        body.size = buf.size() - offset;
        boost::system::error_code ec{};
        http::read(stream, flat_buf, parser, ec);
        if (!ec || ec == http::error::need_buffer) {
            const auto unused = body.size;
            buf.resize(buf.size() - unused);
            ec = {};
        }
        return ec;
    }

    void respond(const Response& resp) {
        boost::system::error_code ec{};
        http::write(stream, make_boost_response(resp, http_version), ec);
        if (ec) PLAI_WARN("failed to respond: '{}'", ec.message());
    }
};

// wrapper for beast parser, allows lazy reading
class RequestImpl final : public Request {
    Method m_verb{};
    const Target* m_tgt{};
    ParsingCtx* m_ctx{};
    QueryParams* m_params{};

 public:
    explicit RequestImpl(Method verb, const Target& tgt, ParsingCtx& ctx,
                         QueryParams& params) noexcept
        : m_verb{verb}, m_tgt(&tgt), m_ctx(&ctx), m_params(&params) {}

    Method method() const noexcept override { return m_verb; }

    const Target& target() const noexcept override { return *m_tgt; }
    std::string_view text() const override {
        while (!m_ctx->is_done()) {
            auto ec = m_ctx->read_next();
            if (ec) throw boost::system::system_error(ec);
        }
        return {reinterpret_cast<const char*>(m_ctx->buf.data()),
                m_ctx->buf.size()};
    }

    const QueryParams& query_params() const override { return *m_params; }

    std::optional<std::string_view> text_chunked() const override {
        m_ctx->buf.clear();
        if (m_ctx->is_done()) return std::nullopt;
        auto ec = m_ctx->read_next();
        if (ec) throw boost::system::system_error(ec);
        return std::string_view{
            reinterpret_cast<const char*>(m_ctx->buf.data()),
            m_ctx->buf.size()};
    }
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
        auto ctx = ParsingCtx(
            beast::basic_stream<local::stream_protocol>(std::move(sock)));

        ctx.read_header();
        auto url_str = std::string(ctx.parser.get().target());
        auto [tgt_str, param_str] = split_left(url_str, "?");
        auto verb = convert_boost_verb(ctx.parser.get().method());
        PLAI_DEBUG(
            "transfer encoding: {}",
            std::string_view(ctx.parser.get()[http::field::transfer_encoding]));
        if (!verb) {
            ctx.respond(
                {.body = "Unsupported method", .status_code = PLAI_HTTP(405)});
            return;
        }
        for (const auto& [key, handler] : m_services) {
            if (!(key.methods & *verb)) continue;
            auto tgt = parse_target(key.pattern, tgt_str);
            if (tgt) {
                PLAI_DEBUG("handler: '{}'", key.pattern);
                auto query_params = parse_query_params(param_str);
                RequestImpl req{*verb, *tgt, ctx, query_params};
                auto resp = handler(req);
                ctx.respond(resp);
            }
        }
        ctx.respond({.body = "Not found", .status_code = PLAI_HTTP(404)});
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
