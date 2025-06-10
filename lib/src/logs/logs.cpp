#include <cstdarg>
#include <mutex>
#include <plai/logs/logs.hpp>
#include <plai/util/cast.hpp>
#include <utility>

#include "plai/format.hpp"

extern "C" {
#include <libavutil/avutil.h>
}

namespace plai::logs {
namespace {
namespace logs_detail {
Level g_level{};
std::unique_ptr<std::FILE, void (*)(std::FILE*)> g_log_stream{
    stderr, [](auto* /*unused*/) {}};

constexpr std::string_view lvl_to_str(Level l) {
    using enum Level;
    switch (l) {
        case Trace: return "TRAC";
        case Debug: return "DEBU";
        case Info: return "INFO";
        case Note: return "NOTE";
        case Warn: return "WARN";
        case Err: return "ERRO";
        case Fatal: return "FATA";
        case Quiet: return "QUIE";
    }
    std::unreachable();
}
void ffmpeg_log_cb(void* /*unused*/, int /*level*/, const char* fmt,
                   std::va_list args) noexcept {
    static std::mutex mut{};
    std::unique_lock lk{mut};
    static std::string buf{};
    try {
        auto stp = SystemClock::now();
        auto tp = Clock::now();
        std::va_list args_cpy{};
        va_copy(args_cpy, args);
        auto res = std::vsnprintf(buf.data(), buf.size(), fmt, args_cpy);
        va_end(args_cpy);
        if (res <= 0) return;
        if (res >= buf.size()) {
            buf = std::string(res + 1, '\0');
            res = std::vsnprintf(buf.data(), buf.size(), fmt, args);
            if (res < 0) return;
        }
        buf[res - 1] = '\0';
        detail::push_log(Level::Trace, stp, tp, buf);
    } catch (...) {}
}
}  // namespace logs_detail

}  // namespace

void init(Level lvl, const std::filesystem::path& logfile) {
    logs_detail::g_level = lvl;
    // TODO: use level
    if (lvl <= Level::Trace) {
        // av_log_set_callback(&logs_detail::ffmpeg_log_cb);
    }
    if (logfile != "-" && !logfile.empty()) {
        logs_detail::g_log_stream =
            std::unique_ptr<std::FILE, void (*)(std::FILE*)>(
                std::fopen(logfile.native().c_str(), "w"),
                [](std::FILE* f) { std::fclose(f); });
    }
}

namespace detail {
void push_log(Level lvl, SystemTimePoint stp, TimePoint tp, std::string msg) {
    const auto lvl_name = logs_detail::lvl_to_str(lvl);
    plai::println(logs_detail::g_log_stream.get(), "{:%F %T} [{}] {}",
                  std::chrono::floor<std::chrono::milliseconds>(stp), lvl_name,
                  msg);
    auto* fd = logs_detail::g_log_stream.get();
    // flushing is potentially very slow so only flush when wrinting to a file
    if (fd != stdout && fd != stderr)
        std::fflush(logs_detail::g_log_stream.get());
}

Level level() noexcept { return logs_detail::g_level; }
}  // namespace detail

}  // namespace plai::logs
