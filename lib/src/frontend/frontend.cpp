#include <plai/frontend/frontend.hpp>
#include <utility>

#include "sdl2.hpp"

namespace plai {

class VoidTexture : public Texture {
 public:
    void blend_mode(BlendMode mode) override {}
    void alpha(uint8_t alpha) override {}
    void render_rgb(uint8_t r, uint8_t g, uint8_t b) override {}
    void update(const media::Frame& frame) override {}

    void render_to(const RenderTarget& tgt) override {}
};

class VoidFrontend final : public Frontend {
 public:
    std::optional<Event> poll_event() override { return std::nullopt; }

    Vec<int> dimensions() override { return {1920, 1080}; }

    std::unique_ptr<Texture> texture() override {
        return std::make_unique<VoidTexture>();
    }

    void render_current() override {}
};

std::unique_ptr<Frontend> frontend(FrontendType type) {
    using enum FrontendType;
    switch (type) {
        case Sdl2: return sdl::sdl_frontend();
        case Void: return std::make_unique<VoidFrontend>();
    }
    std::unreachable();
}
}  // namespace plai
