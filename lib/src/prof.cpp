#include <cassert>
#include <plai/prof.hpp>

namespace plai {

// NOLINTNEXTLINE
ProfileStatistics* g_stats{};

void profiling_statistics(ProfileStatistics* stats) { g_stats = stats; }

Profiler profile(std::string_view nm) { return profile(nm, g_stats); }

Profiler profile(std::string_view nm, ProfileStatistics* stats) {
    assert(stats);
    return {stats, stats->index(nm)};
}
}  // namespace plai
