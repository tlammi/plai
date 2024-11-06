#include <cassert>
#include <cstdio>
#include <plai/exceptions.hpp>
#include <plai/fs/read.hpp>
#include <plai/util/defer.hpp>
#include <print>

namespace plai::fs {

std::vector<uint8_t> read_bin(const stdfs::path& path) {
    // NOLINTNEXTLINE
    FILE* f = fopen(path.native().c_str(), "r");
    if (!f) {
        throw ValueError(std::format("could not open {}", path.native()));
    }
    // NOLINTNEXTLINE
    auto defer = Defer([&] { fclose(f); });
    fseek(f, 0, SEEK_END);
    auto size = ftell(f);
    rewind(f);
    auto vec = std::vector<uint8_t>(size);
    auto read_bytes = fread(vec.data(), sizeof(uint8_t), vec.size(), f);
    assert(read_bytes == vec.size());
    std::println("read {} bytes", read_bytes);
    return vec;
}
}  // namespace plai::fs
