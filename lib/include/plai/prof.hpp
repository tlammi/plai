#pragma once

#include <algorithm>
#include <mutex>
#include <plai/time.hpp>
#include <string_view>
#include <vector>

namespace plai {

class ProfilingCtx {
 public:
    ProfilingCtx() {}

 private:
};

struct Profile {
    FloatDuration min{FloatDuration::max()};
    FloatDuration avg{0};
    FloatDuration max{FloatDuration::min()};
    FloatDuration latest{0};
    size_t measurements{};

    constexpr void update(Duration meas) noexcept {
        auto dur = std::chrono::duration_cast<FloatDuration>(meas);
        min = std::min(min, dur);
        max = std::max(max, dur);
        latest = dur;
        const auto m = measurements;
        avg = (dur + m * avg) / double(m + 1);
        ++measurements;
    }
};

class ProfileStatistics {
 public:
    size_t index(std::string_view nm) {
        auto lk = std::unique_lock(m_mut);
        auto iter =
            std::find_if(m_prof.begin(), m_prof.end(),
                         [&](const auto& pair) { return pair.first == nm; });
        if (iter == m_prof.end()) {
            m_prof.emplace_back(std::string(nm), Profile{});
            return m_prof.size() - 1;
        }
        return iter - m_prof.begin();
    }

    Profile operator[](size_t idx) {
        auto lk = std::unique_lock(m_mut);
        return m_prof.at(idx).second;
    }

    Profile operator[](std::string_view nm) { return operator[](index(nm)); }
    void add(size_t idx, Duration meas) { m_prof.at(idx).second.update(meas); }

 private:
    std::mutex m_mut{};
    std::vector<std::pair<std::string, Profile>> m_prof{};
};

class Profiler {
 public:
    Profiler(ProfileStatistics* stat, size_t idx)
        : m_stat(stat), m_idx(idx), m_start(Clock::now()) {}

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    Profiler(Profiler&&) = delete;
    Profiler& operator=(Profiler&&) = delete;

    ~Profiler() {
        if (m_stat) m_stat->add(m_idx, Clock::now() - m_start);
    }

 private:
    ProfileStatistics* m_stat;
    size_t m_idx;
    TimePoint m_start{};
};

void profiling_statistics(ProfileStatistics* stats);

[[nodiscard]] Profiler profile(std::string_view nm);
[[nodiscard]] Profiler profile(std::string_view nm, ProfileStatistics* stats);

}  // namespace plai
