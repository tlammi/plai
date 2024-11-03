#pragma once

#include <plai/frac.hpp>
#include <plai/media/forward.hpp>

namespace plai::media {

/**
 * \brief View to demultiplexer's stream
 *
 * This is a pointer to demultiplexer's internal data so it is only
 * valid as long as the demultiplexer is.
 * */
class StreamView {
 public:
    constexpr StreamView() noexcept = default;
    constexpr explicit StreamView(AVStream* raw) noexcept : m_raw(raw) {}

    [[nodiscard]] bool video() const noexcept;
    [[nodiscard]] bool audio() const noexcept;

    constexpr AVStream* raw() noexcept { return m_raw; }
    constexpr const AVStream* raw() const noexcept { return m_raw; }

    Frac<int> fps() const noexcept;

 private:
    AVStream* m_raw{};
};
}  // namespace plai::media
