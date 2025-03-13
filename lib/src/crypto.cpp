#include <openssl/evp.h>
#include <openssl/sha.h>

#include <plai/crypto.hpp>

namespace plai::crypto {

Sha256 sha256(std::span<const uint8_t> data) {
    const auto* algo = EVP_sha256();
    Sha256 out{};
    unsigned size{};
    const int res =
        EVP_Digest(data.data(), data.size(), out.data(), &size, algo, nullptr);
    if (!res) {}
    return out;
}

Sha256 sha256(std::string_view data) {
    auto span = std::span(
        reinterpret_cast<const uint8_t*>(data.data()) /*NOLINT*/, data.size());
    return sha256(span);
}

std::string hex_str(Sha256View data) {
    std::string res{};
    for (const uint8_t byte : data) {
        auto lower = byte & 0x0f;
        auto upper = (byte & 0xf0) >> 4;
        char lower_c = lower < 10 ? lower + '0' : lower - 10 + 'a';
        char upper_c = upper < 10 ? upper + '0' : upper - 10 + 'a';
        res.push_back(upper_c);
        res.push_back(lower_c);
    }
    return res;
}
}  // namespace plai::crypto
