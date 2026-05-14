#pragma once

#include <plai/exceptions.hpp>

namespace plai::net::http {

class Exception : public std::exception {
    struct Args {
        unsigned status_code;
        std::string body;
        std::string content_type{};
    };

 public:
    explicit Exception(Args args) noexcept : m_args(std::move(args)) {}

    const Args& args() const noexcept { return m_args; }
    Args m_args;

 private:
};
}  // namespace plai::net::http
