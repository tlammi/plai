#pragma once

#include <cstdint>
#include <plai/net/http/macros.hpp>
#include <string>

namespace plai::net::http {

struct Response {
    std::string body;
    uint16_t status_code{PLAI_HTTP(200)};
};
}  // namespace plai::net::http
