#pragma once

#include <plai/frontend/type.hpp>
#include <plai/logs/logs.hpp>
#include <plai/time.hpp>
#include <string>

namespace plaibin {

struct Cli {
    std::string db;
    std::string socket;
    std::string watermark;
    plai::RenderTarget watermark_tgt{};
    plai::Duration blend;
    plai::Duration img_dur{};
    bool void_frontend = false;
    plai::logs::Level log_level{plai::logs::Level::Info};
    bool fullscreen{false};
};

class Exit : public std::exception {
 public:
    Exit(int code) : m_code(code) {}

    int code() const noexcept { return m_code; }

 private:
    int m_code;
};

Cli parse_cli(int argc, char** argv);

}  // namespace plaibin
