#pragma once

#include <memory>
#include <plai/frontend/type.hpp>
#include <plai/media/frame.hpp>
#include <vector>

namespace plai::frontend {

struct RenderOpts {
    double render_w = 1.0;
    double render_h = 1.0;
    std::uint8_t alpha{};
    Position vertical : 2 = Position::Begin;
    Position horizontal : 2 = Position::Begin;
    Scaling scaling : 1 = Scaling::Fit;
    BlendMode blend_mode : 1 = BlendMode::None;
    bool active : 1 = false;
};

class Renderer {
 public:
    class Device {
        friend Renderer;

     public:
        constexpr virtual ~Device() = default;
        virtual void upload_texture(size_t idx, const media::Frame& frm) = 0;
        virtual void swap_textures(size_t idx_a, size_t idx_b) = 0;
        virtual void update() = 0;

     private:
        virtual void set_renderer(Renderer& rend) = 0;
    };

    Renderer(size_t texture_count, std::unique_ptr<Device> dev)
        : m_dev(std::move(dev)), m_opts(texture_count) {}

    std::span<RenderOpts> render_opts() noexcept { return m_opts; }

    void upload_texture(size_t idx, const media::Frame& frm) {
        m_dev->upload_texture(idx, frm);
    }

    void swap_textures(size_t idx_a, size_t idx_b) {
        m_dev->swap_textures(idx_a, idx_b);
    }

    void update() { m_dev->update(); }

    Device& device() const noexcept { return *m_dev; }

 private:
    std::unique_ptr<Device> m_dev;
    std::vector<RenderOpts> m_opts;
};

Renderer make_renderer(size_t texture_count, FrontendType frontend_type);

}  // namespace plai::frontend
