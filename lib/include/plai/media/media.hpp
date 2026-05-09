#pragma once
#include <cstdint>
#include <memory>
#include <ranges>
#include <span>
#include <variant>
#include <vector>

namespace plai::media {
namespace media_detail {

constexpr auto to_uint8t_vec(const auto& in) {
    return in | std::views::transform([](std::byte b) {
               return static_cast<uint8_t>(b);
           }) |
           std::ranges::to<std::vector>();
}

}  // namespace media_detail

class Media {
 public:
    constexpr Media() noexcept = default;

    explicit Media(std::vector<uint8_t> v)
        : m_dat(std::make_shared<std::vector<uint8_t>>(std::move(v))) {}

    explicit Media(const std::vector<std::byte>& v)
        : Media(media_detail::to_uint8t_vec(v)) {}

    std::span<uint8_t> data() const noexcept { return *m_dat; }

    std::vector<uint8_t> get_data() { return std::move(*m_dat); }

    constexpr explicit operator bool() const noexcept {
        return static_cast<bool>(m_dat);
    }

 private:
    std::shared_ptr<std::vector<uint8_t>> m_dat{};
};

}  // namespace plai::media
