#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace plai::crypto {

constexpr uint8_t SHA256_LEN = 32;
using Sha256 = std::array<uint8_t, SHA256_LEN>;
using Sha256View = std::span<const uint8_t, SHA256_LEN>;

Sha256 sha256(std::span<const uint8_t> data);
Sha256 sha256(std::string_view data);

std::string hex_str(Sha256View data);

}  // namespace plai::crypto
