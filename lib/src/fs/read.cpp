#include <cassert>
#include <cstdio>
#include <plai/exceptions.hpp>
#include <plai/format.hpp>
#include <plai/fs/read.hpp>
#include <plai/util/defer.hpp>

namespace plai::fs {

std::vector<uint8_t> read_bin(const stdfs::path& path) {
    // NOLINTNEXTLINE
    FILE* f = fopen(path.native().c_str(), "r");
    if (!f) {
        throw ValueError(plai::format("could not open {}", path.native()));
    }
    // NOLINTNEXTLINE
    auto defer = Defer([&] { fclose(f); });
    fseek(f, 0, SEEK_END);
    auto size = ftell(f);
    rewind(f);
    auto vec = std::vector<uint8_t>(size);
    auto read_bytes = fread(vec.data(), sizeof(uint8_t), vec.size(), f);
    assert(read_bytes == vec.size());
    return vec;
}
std::generator<std::span<uint8_t>> read_chunked(
    const stdfs::path& path /*NOLINT(*-avoid-reference*)*/) {
    static constexpr auto buf_size = 1024;
    std::array<uint8_t, buf_size> buf{};
    // NOLINTNEXTLINE
    FILE* f = fopen(path.native().c_str(), "r");
    if (!f) throw ValueError(plai::format("could not open {}", path.native()));
    // NOLINTNEXTLINE
    auto defer = Defer([&] { fclose(f); });

    while (true) {
        auto count = fread(buf.data(), sizeof(uint8_t), buf.size(), f);
        if (count == buf.size()) co_yield buf;
        if (std::feof(f)) {
            co_yield buf;
            co_return;
        }
        throw ValueError(plai::format("error reading file {}", path.native()));
    }
}

}  // namespace plai::fs
