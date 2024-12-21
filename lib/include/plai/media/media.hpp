#pragma once
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

namespace plai::media {

struct Video {
    std::vector<uint8_t> data;
};
struct Image {
    std::vector<uint8_t> data;
};

// careful with these
struct VideoView {
    std::span<const uint8_t> data;
};

struct ImageView {
    std::span<const uint8_t> data;
};

using Media = std::variant<Video, Image>;

}  // namespace plai::media
