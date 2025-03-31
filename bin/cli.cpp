#include "cli.hpp"

#include <CLI/CLI.hpp>

namespace plaibin {

Cli parse_cli(int argc, char** argv) {
    auto parser = CLI::App("simple media player");
    argv = parser.ensure_utf8(argv);
    Cli out{.db = ":memory:",
            .socket = "/tmp/plai.sock",
            .watermark = "",
            .blend = std::chrono::seconds(2)};
    parser.add_option("-d,--db", out.db,
                      "Database path. Use ':memory:' for in-memory database. "
                      "Default: ':memory:'.");
    parser.add_option(
        "-s,--socket", out.socket,
        std::format("Path to API unix socket. Default '{}'", out.socket));
    parser.add_option(
        "-w,--watermark", out.watermark,
        "Place a watermark to the player. Empty string to disable.");
    parser.add_option(
        "-b,--blend", out.blend,
        std::format("Media blend duration. Default: {}", out.blend));
    parser.add_flag("--void", out.void_frontend,
                    "Use the void frontend, i.e. discard the output");
    try {
        parser.parse(argc, argv);
    } catch (const CLI::ParseError& e) { throw Exit(parser.exit(e)); }
    return out;
}
}  // namespace plaibin
