#pragma once

#include <cstdint>

namespace plai::net::http {

using Method = std::uint8_t;

constexpr Method METHOD_GET = 0x01;
constexpr Method METHOD_POST = 0x02;
constexpr Method METHOD_PUT = 0x04;
constexpr Method METHOD_DELETE = 0x08;

}  // namespace plai::net::http
