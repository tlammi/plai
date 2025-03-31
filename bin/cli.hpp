#pragma once

#include <plai/time.hpp>
#include <string>

namespace plaibin {

struct Cli {
    std::string db;
    std::string socket;
    std::string watermark;
    plai::Duration blend;
    bool void_frontend = false;
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
