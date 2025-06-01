#include "cli.hpp"

#include <CLI/CLI.hpp>

namespace plaibin {
namespace {
enum class WatermarkPos {
    Tl,
    Tm,
    Tr,
    Ml,
    Mm,
    Mr,
    Bl,
    Bm,
    Br,
};

constexpr plai::Position pos_horizontal(WatermarkPos in) {
    using enum WatermarkPos;
    using enum plai::Position;
    switch (in) {
        case WatermarkPos::Tl:
        case WatermarkPos::Ml:
        case WatermarkPos::Bl: return Begin;
        case WatermarkPos::Tm:
        case WatermarkPos::Mm:
        case WatermarkPos::Bm: return Middle;
        case WatermarkPos::Tr:
        case WatermarkPos::Mr:
        case WatermarkPos::Br: return End;
    }
    std::unreachable();
}

constexpr plai::Position pos_vertical(WatermarkPos in) {
    using enum WatermarkPos;
    using enum plai::Position;
    switch (in) {
        case WatermarkPos::Tl:
        case WatermarkPos::Tm:
        case WatermarkPos::Tr: return Begin;
        case WatermarkPos::Ml:
        case WatermarkPos::Mm:
        case WatermarkPos::Mr: return Middle;
        case WatermarkPos::Bl:
        case WatermarkPos::Bm:
        case WatermarkPos::Br: return End;
    }
    std::unreachable();
}

constexpr plai::Duration to_duration(double secs) noexcept {
    namespace sc = std::chrono;
    return sc::duration_cast<plai::Duration>(sc::duration<double>(secs));
}

}  // namespace

Cli parse_cli(int argc, char** argv) {
    auto parser = CLI::App("simple media player");
    argv = parser.ensure_utf8(argv);
    Cli out{
        .db = ":memory:",
        .socket = "/tmp/plai.sock",
        .watermark = "",
        .blend = std::chrono::seconds(2),
        .img_dur = std::chrono::seconds(1),
    };
    double img_dur = 1.0;
    double blend = 1.0;
    WatermarkPos wm_pos{WatermarkPos::Bl};
    using enum plai::logs::Level;
    const std::map<std::string, plai::logs::Level> log_mapping{{"trace", Trace},
                                                               {"debug", Debug},
                                                               {"info", Info},
                                                               {"warn", Warn},
                                                               {"error", Err}};
    using enum WatermarkPos;
    const std::map<std::string, WatermarkPos> wm_pos_mapping{
        {"tl", Tl}, {"tm", Tm}, {"tr", Tr}, {"ml", Ml}, {"mm", Mm},
        {"mr", Mr}, {"bl", Bl}, {"bm", Bm}, {"br", Br},
    };

    parser.add_option("-l,--loglevel", out.log_level, "Log level")
        ->transform(CLI::CheckedTransformer(log_mapping, CLI::ignore_case));
    parser.add_option("--logfile", out.log_file,
                      "Log file path. If '-' (default) or '' stderr is used");
    parser.add_option("-d,--db", out.db,
                      "Database path. Use ':memory:' for in-memory database. "
                      "Default: ':memory:'.");
    parser.add_option(
        "-s,--socket", out.socket,
        plai::format("Path to API unix socket. Default '{}'", out.socket));
    parser.add_option(
        "-w,--watermark", out.watermark,
        "Place a watermark to the player. Empty string to disable.");
    parser.add_option("--watermark-w", out.watermark_tgt.w,
                      "Watermark width scaling");
    parser.add_option("--watermark-h", out.watermark_tgt.h,
                      "Watermark height scaling");
    parser.add_option("--watermark-pos", wm_pos, "Watermark position")
        ->transform(CLI::CheckedTransformer(wm_pos_mapping, CLI::ignore_case));
    bool stretch = false;
    parser.add_flag("--watermark-stretch,!--no-watermark-stretch", stretch,
                    "Whether to stretch watermark");
    parser.add_option(
        "--accel", out.accel,
        "Hardware acceleration to use. 'sw' for software (default).");

    parser.add_option(
        "-b,--blend", blend,
        plai::format("Media blend duration in seconds. Default: {}", blend));
    parser.add_option(
        "--img-dur", img_dur,
        plai::format("Duration to show images in seconds. Default: {}",
                     img_dur));
    parser.add_flag("--fullscreen,!--no-fullscreen", out.fullscreen,
                    "Start with fullscreen enabled");
    parser.add_flag("--void", out.void_frontend,
                    "Use the void frontend, i.e. discard the output");
    parser.add_flag("--list-accel", out.list_accel,
                    "List available accelerators");
    try {
        parser.parse(argc, argv);
    } catch (const CLI::ParseError& e) { throw Exit(parser.exit(e)); }
    out.watermark_tgt.vertical = pos_vertical(wm_pos);
    out.watermark_tgt.horizontal = pos_horizontal(wm_pos);
    out.watermark_tgt.scaling =
        stretch ? plai::Scaling::Stretch : plai::Scaling::Fit;
    out.blend = to_duration(blend);
    out.img_dur = to_duration(img_dur);
    return out;
}
}  // namespace plaibin
