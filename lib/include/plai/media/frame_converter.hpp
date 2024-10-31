#pragma once

#include <memory>
#include <plai/media/forward.hpp>
#include <plai/media/frame.hpp>
#include <plai/vec.hpp>

namespace plai::media {

/**
 * \brief Convert frames from one format to another
 * */
class FrameConverter {
 public:
    FrameConverter(Vec<int> dst, bool adjust_pixels = true);

    Frame operator()(const Frame& input);

 private:
    std::unique_ptr<SwsContext, void (*)(SwsContext*)> m_ctx;
    Vec<int> m_dst{};
    bool m_pix_adjust{true};
};
}  // namespace plai::media
