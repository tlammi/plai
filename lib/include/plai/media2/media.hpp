#pragma once

#include <memory>
#include <span>
#include <vector>

namespace plai::media2 {

/**
 * \brief Reference counted media data
 * */
class Media {
 public:
    constexpr explicit Media() noexcept = default;
    explicit Media(std::vector<std::byte> vec) : m_dat(new std::vector(vec)) {}
    std::span<const std::byte> data() const noexcept { return *m_dat; }

    constexpr explicit operator bool() const noexcept {
        return m_dat != nullptr;
    }

    constexpr void reset() noexcept { m_dat.reset(); }

 private:
    // TODO: Small optimization here by inlining vector here
    // to shared_ptr's storage
    std::shared_ptr<const std::vector<std::byte>> m_dat{};
};
}  // namespace plai::media2
