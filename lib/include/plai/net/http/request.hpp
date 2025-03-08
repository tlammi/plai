#pragma once

#include <cstdint>
#include <plai/net/http/target.hpp>
#include <vector>

namespace plai::net::http {

class Request {
 public:
    virtual const Target& target() const = 0;

    virtual std::string_view text() const = 0;

 protected:
    constexpr ~Request() = default;
};
}  // namespace plai::net::http
