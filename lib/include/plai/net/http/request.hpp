#pragma once

#include <cstdint>
#include <plai/net/http/target.hpp>
#include <vector>

namespace plai::net::http {

struct Request {
    Target target{};
    std::string_view body;
};
}  // namespace plai::net::http
