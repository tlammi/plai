#pragma once

#include <format>
#include <plai/logs/level.hpp>
#include <plai/time.hpp>

namespace plai::logs {

void init(Level lvl);

namespace detail {
void push_log(Level lvl, TimePoint tp, std::string msg);
}

#define PLAI_LOG(lvl, fmt, ...)                                          \
  ::plai::logs::detail::push_log(::plai::logs::Level::lvl, Clock::now(), \
                                 std::format(fmt "\n", __VA_ARGS__))

#define PLAI_TRACE(...) PLAI_LOG(Trace, __VA_ARGS__)
#define PLAI_DEBUG(...) PLAI_LOG(Debug, __VA_ARGS__)
#define PLAI_INFO(...) PLAI_LOG(Info, __VA_ARGS__)
#define PLAI_NOTE(...) PLAI_LOG(Note, __VA_ARGS__)
#define PLAI_WARN(...) PLAI_LOG(Warn, __VA_ARGS__)
#define PLAI_ERR(...) PLAI_LOG(Err, __VA_ARGS__)
#define PLAI_FATAL(...) PLAI_LOG(Fatal, __VA_ARGS__)

}  // namespace plai::logs
