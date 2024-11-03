#pragma once

#include <cstdint>

namespace plai::net::http::method {

using Method = uint8_t;

constexpr Method GET = 1;
constexpr Method POST = 1 << 1;
constexpr Method PUT = 1 << 2;
constexpr Method DELETE = 1 << 3;

}  // namespace plai::net::http::method
