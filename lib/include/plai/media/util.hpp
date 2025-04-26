#pragma once

#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <plai/media/frame.hpp>

namespace plai::media {

/**
 * \brief Decode the first frame from the given media
 * */
Frame decode_image(std::span<const uint8_t> data);
Frame decode_image(const std::filesystem::path& path);
}  // namespace plai::media
