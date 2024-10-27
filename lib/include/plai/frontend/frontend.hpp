#pragma once

#include <plai/frontend/texture.hpp>
#include <plai/frontend/type.hpp>
#include <plai/frontend/window.hpp>

namespace plai {

/**
 * \brief Frontend
 *
 * This is what actually displays the window etc.
 * */
class Frontend {
 public:
    virtual ~Frontend() = default;

    /**
     * \brief Create a new texture
     *
     * Creates a new texture with the given render target options. The same
     * texture is used for rendering all the targets.
     * */
    virtual std::unique_ptr<Texture> texture(
        std::span<RenderTargetOpts> target_opts) = 0;

    /**
     * \brief Texture with single render target
     * */
    std::unique_ptr<Texture> texture1(RenderTargetOpts target_opts) {
        return texture(std::span(&target_opts, 1));
    }

    virtual void render_current() = 0;

 private:
    std::unique_ptr<Window> m_win;
};

std::unique_ptr<Frontend> frontend(FrontendType type);
inline std::unique_ptr<Frontend> frontend(std::string_view nm) {
    return frontend(frontend_type(nm));
}

}  // namespace plai
