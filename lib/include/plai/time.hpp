#pragma once

#include <chrono>
namespace plai {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

using SystemClock = std::chrono::system_clock;
using SystemTimePoint = SystemClock::time_point;

}  // namespace plai
