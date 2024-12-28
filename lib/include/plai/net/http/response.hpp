#pragma once

#include <cstdint>
#include <plai/net/http/macros.hpp>
#include <vector>

namespace plai::net::http {

struct Response {
    std::vector<uint8_t> body;
    uint8_t status_code{PLAI_HTTP(200)};
};
}  // namespace plai::net::http
