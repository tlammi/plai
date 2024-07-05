#include <cstdarg>
#include <plai/logs/logs.hpp>
#include <plai/util/cast.hpp>
#include <print>
#include <utility>

extern "C" {
#include <libavutil/avutil.h>
}

namespace plai::logs {
namespace {
namespace logs_detail {
constexpr std::string_view lvl_to_str(Level l) {
  using enum Level;
  switch (l) {
    case Trace:
      return "TRAC";
    case Debug:
      return "DEBU";
    case Info:
      return "INFO";
    case Note:
      return "NOTE";
    case Warn:
      return "WARN";
    case Err:
      return "ERRO";
    case Fatal:
      return "FATA";
    case Quiet:
      return "QUIE";
  }
  std::unreachable();
}
}  // namespace logs_detail

}  // namespace

Level g_level{};

void init(Level lvl) {
  g_level = lvl;
  // TODO: use level
  if (lvl <= Level::Trace) {
    auto cb = +[](void* /*unused*/, int lvl, const char* fmt,
                  std::va_list args) { std::vfprintf(stderr, fmt, args); };
    av_log_set_callback(cb);
  }
}

namespace detail {
void push_log(Level lvl, SystemTimePoint stp, TimePoint tp, std::string msg) {
  const auto lvl_name = logs_detail::lvl_to_str(lvl);
  std::println(stderr, "{:%F %T} [{}] {}",
               std::chrono::floor<std::chrono::milliseconds>(stp), lvl_name,
               msg);
}

Level level() noexcept { return g_level; }
}  // namespace detail

}  // namespace plai::logs
