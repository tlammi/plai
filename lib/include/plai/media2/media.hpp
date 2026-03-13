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
    explicit Media(std::vector<std::byte> vec) : m_dat(new std::vector(vec)) {}
    std::span<const std::byte> data() const noexcept { return *m_dat; }

 private:
    // TODO: Small optimization here by inlining vector here
    // to shared_ptr's storage
    std::shared_ptr<const std::vector<std::byte>> m_dat{};
};
}  // namespace plai::media2
