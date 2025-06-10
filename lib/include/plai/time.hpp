#pragma once

#include <chrono>
#include <plai/concepts.hpp>
#include <thread>

namespace plai {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;
using FloatDuration = std::chrono::duration<double>;

using SystemClock = std::chrono::system_clock;
using SystemTimePoint = SystemClock::time_point;

namespace time_detail {
// Time to busy wait after sleep
constexpr Clock::duration EARLY_WAKEUP = std::chrono::microseconds(1000);
// Time to return early from a sleep
constexpr Clock::duration EARLY_RETURN = std::chrono::nanoseconds(100);
}  // namespace time_detail

/**
 * \brief High precision sleep
 * */
template <class Tp>
void precision_sleep_until(Tp tp) {
    const auto deep_sleep = tp - time_detail::EARLY_WAKEUP;
    const auto busy_wait = tp - time_detail::EARLY_RETURN;
    std::this_thread::sleep_until(deep_sleep);
    while (Clock::now() < busy_wait);
}

/**
 * \brief High precision sleep
 * */
template <class Dur>
void precision_sleep_for(Dur&& dur) {
    return precision_sleep_until(Clock::now() + std::forward<Dur>(dur));
}

class RateLimiter {
 public:
    explicit RateLimiter(Duration period, TimePoint start = Clock::now())
        : m_period(period), m_prev(start) {}

    void operator()() {
        m_prev += m_period;
        precision_sleep_until(m_prev);
    }

    auto& period() noexcept { return m_period; }
    const auto& period() const noexcept { return m_period; }

 private:
    Duration m_period;
    TimePoint m_prev;
};

}  // namespace plai
