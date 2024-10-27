#pragma once

#include <cstdint>
#include <plai/frontend/type.hpp>
#include <plai/media/frame.hpp>

namespace plai {

class Texture {
 public:
    virtual ~Texture() = default;

    virtual void blend_mode(BlendMode mode) = 0;
    virtual void alpha(uint8_t alpha) = 0;
    virtual void render_rgb(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void render_frame(const media::Frame& frame) = 0;

 private:
};

}  // namespace plai
