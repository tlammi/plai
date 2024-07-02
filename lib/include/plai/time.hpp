#pragma once

#include <chrono>
namespace plai {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

}  // namespace plai
