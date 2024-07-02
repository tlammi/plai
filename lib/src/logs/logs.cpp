#include <cstdarg>
#include <plai/logs/logs.hpp>
#include <plai/util/cast.hpp>

extern "C" {
#include <libavutil/avutil.h>
}

namespace plai::logs {

void init(Level lvl) {
  // TODO: use level
  // TODO: use level for av_log*
  (void)lvl;
  auto cb = +[](void* /*unused*/, int lvl, const char* fmt, std::va_list args) {
    std::vfprintf(stderr, fmt, args);
  };
  av_log_set_callback(cb);
}
namespace detail {
void push_log(Level lvl, TimePoint tp, std::string msg) {
  // TODO: print level
  // TODO: use timepoint
  std::fputs(msg.c_str(), stderr);
}
}  // namespace detail

}  // namespace plai::logs
