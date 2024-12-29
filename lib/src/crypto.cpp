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

}  // namespace plai::crypto
