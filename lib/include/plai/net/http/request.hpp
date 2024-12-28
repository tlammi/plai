#pragma once

#include <cstdint>
#include <plai/net/http/target.hpp>
#include <vector>

namespace plai::net::http {

struct Request {
    Target target{};
    std::span<const uint8_t> body;
};
}  // namespace plai::net::http
