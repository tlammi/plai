#include <gtest/gtest.h>

#include <plai/prof.hpp>

TEST(Profiling, NoOp) {
    auto s = plai::ProfileStatistics();
    auto profiler = plai::profile("foo", &s);
}

TEST(Profiling, Global) {
    auto stats = plai::ProfileStatistics();
    plai::profiling_statistics(&stats);
    { auto profiler = plai::profile("foo"); }
}

TEST(Profiling, Single) {
    auto s = plai::ProfileStatistics();
    { auto p = plai::profile("foo", &s); }
    auto prof = s["foo"];
    ASSERT_EQ(prof.measurements, 1);
    ASSERT_EQ(prof.min, prof.latest);
    ASSERT_EQ(prof.max, prof.latest);
    ASSERT_EQ(prof.avg, prof.latest);
}

