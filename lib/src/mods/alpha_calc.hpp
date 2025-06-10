#pragma once

#include <limits>
#include <plai/time.hpp>
namespace plai::mods {

class AlphaCalc {
 public:
    constexpr AlphaCalc() noexcept = default;

    constexpr explicit AlphaCalc(Duration duration,
                                 TimePoint start = Clock::now()) noexcept
        : m_dur(duration), m_start(start) {}

    constexpr uint8_t operator()(TimePoint now = Clock::now()) const noexcept {
        auto delta = now - m_start;
        auto ratio = std::chrono::duration_cast<FloatDuration>(delta) / m_dur;
        ratio = std::min(ratio, 1.0);
        return static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() *
                                    ratio);
    }

 private:
    FloatDuration m_dur{};
    TimePoint m_start{};
};
}  // namespace plai::mods
