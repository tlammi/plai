#pragma once

#include <httplib.h>

#include <plai/net/http/method.hpp>
#include <print>
#include <string_view>

namespace plai::net::http {

using Request = httplib::Request;
using Response = httplib::Response;

class Server {
    friend class ServerBuilder;

 public:
 private:
    explicit Server(std::unique_ptr<httplib::Server> srv)
        : m_srv(std::move(srv)) {}
    std::unique_ptr<httplib::Server> m_srv;
};

class ServerBuilder {
 public:
    using Self = ServerBuilder;
    Self& prefix(std::string_view prefix) noexcept {
        m_prefix = prefix;
        return *this;
    }

    Self& service(const std::string& pattern, method::Method mthd,
                  std::function<void(const Request&, Response&)> cb) {
        if (mthd & method::GET) m_srv->Get(pattern, cb);
        if (mthd & method::POST) m_srv->Post(pattern, cb);
        return *this;
    }

    void run(const std::string& host, int port) { m_srv->listen(host, port); }

 private:
    std::unique_ptr<httplib::Server> m_srv{std::make_unique<httplib::Server>()};
    using ServicePair =
        std::pair<std::string, std::function<void(const Request&, Response&)>>;
    std::string_view m_prefix{};
};

}  // namespace plai::net::http
