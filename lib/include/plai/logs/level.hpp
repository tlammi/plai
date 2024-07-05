#pragma once

#include <compare>
#include <plai/util/cast.hpp>

namespace plai::logs {
enum class Level {
  Trace,
  Debug,
  Info,
  Note,
  Warn,
  Err,
  Fatal,
  Quiet,
};

constexpr auto operator<=>(Level l, Level r) noexcept {
  return underlying_cast(l) <=> underlying_cast(r);
}

}  // namespace plai::logs
