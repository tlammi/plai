#pragma once

#include <cstdint>
#include <vector>

namespace plai::net::http {

struct Response {
    std::vector<uint8_t> body;
};
}  // namespace plai::net::http
