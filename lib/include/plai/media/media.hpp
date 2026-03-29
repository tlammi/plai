#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <variant>
#include <vector>

namespace plai::media {

class Media {
 public:
    constexpr Media() noexcept = default;

    explicit Media(std::vector<uint8_t> v)
        : m_dat(std::make_shared<std::vector<uint8_t>>(std::move(v))) {}

    std::span<uint8_t> data() const noexcept { return *m_dat; }

    std::vector<uint8_t> get_media() { return std::move(*m_dat); }

 private:
    std::shared_ptr<std::vector<uint8_t>> m_dat{};
};

}  // namespace plai::media
