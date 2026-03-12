#pragma once

#include <cstdint>
#include <filesystem>
#include <generator>
#include <vector>

namespace plai::fs {
namespace stdfs = std::filesystem;

std::vector<uint8_t> read_bin(const stdfs::path& path);
std::generator<std::span<uint8_t>> read_chunked(const stdfs::path& path);

}  // namespace plai::fs
