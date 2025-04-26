#pragma once

#include <functional>
#include <memory>
#include <plai/net/http/method.hpp>
#include <plai/net/http/request.hpp>
#include <plai/net/http/response.hpp>

namespace plai::net::http {

class Server;

class ServerBuilder {
    friend class Server;

 public:
    using Self = ServerBuilder;

    ServerBuilder();

    ~ServerBuilder();

    Self& prefix(std::string prefix);

    Self& service(std::string pattern, Method methods,
                  std::function<Response(const Request&)> handler);

    Self& bind(std::string socket);

    Server commit();

 private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class Server {
    friend class ServerBuilder::Impl;

 public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    Server(Server&&) noexcept;
    Server& operator=(Server&&) noexcept;

    ~Server();

    /**
     * \brief Run synchronously
     * */
    void run();

    void stop();

 private:
    class Impl;
    Server(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

inline ServerBuilder server() noexcept { return {}; }

}  // namespace plai::net::http
