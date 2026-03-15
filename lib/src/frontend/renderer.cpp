#include <SDL2/SDL.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>

#include <mutex>
#include <plai/frontend/exceptions.hpp>
#include <plai/frontend/renderer.hpp>
#include <plai/logs/logs.hpp>

namespace plai::frontend {
namespace detail {
namespace {
inline void sdl_check(int res) {
    if (res < 0) { throw FrontendException(SDL_GetError()); }
}

// NOLINTNEXTLINE
#define SDL_CHECK(expr) \
    do { detail::sdl_check(expr); } while (0)

struct PixelFmtMapEntry {
    AVPixelFormat av;
    SDL_PixelFormatEnum sdl;
};
constexpr auto AV_TO_SDL_PIXEL_FMT_MAP = std::array<PixelFmtMapEntry, 20>{
    PixelFmtMapEntry{AV_PIX_FMT_RGB8, SDL_PIXELFORMAT_RGB332},
    {AV_PIX_FMT_RGB444, SDL_PIXELFORMAT_RGB444},
    {AV_PIX_FMT_RGB555, SDL_PIXELFORMAT_RGB555},
    {AV_PIX_FMT_BGR555, SDL_PIXELFORMAT_BGR555},
    {AV_PIX_FMT_RGB565, SDL_PIXELFORMAT_RGB565},
    {AV_PIX_FMT_BGR565, SDL_PIXELFORMAT_BGR565},
    {AV_PIX_FMT_RGB24, SDL_PIXELFORMAT_RGB24},
    {AV_PIX_FMT_BGR24, SDL_PIXELFORMAT_BGR24},
    {AV_PIX_FMT_0RGB32, SDL_PIXELFORMAT_RGB888},
    {AV_PIX_FMT_0BGR32, SDL_PIXELFORMAT_BGR888},
    {AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888},
    {AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888},
    {AV_PIX_FMT_RGB32, SDL_PIXELFORMAT_ARGB8888},
    {AV_PIX_FMT_RGB32_1, SDL_PIXELFORMAT_RGBA8888},
    {AV_PIX_FMT_BGR32, SDL_PIXELFORMAT_ABGR8888},
    {AV_PIX_FMT_BGR32_1, SDL_PIXELFORMAT_BGRA8888},
    {AV_PIX_FMT_YUV422P, SDL_PIXELFORMAT_IYUV},
    {AV_PIX_FMT_YUV420P, SDL_PIXELFORMAT_IYUV},
    {AV_PIX_FMT_YUYV422, SDL_PIXELFORMAT_YUY2},
    {AV_PIX_FMT_UYVY422, SDL_PIXELFORMAT_UYVY},
    /*{AV_PIX_FMT_YUVJ422P, SDL_PIXELFORMAT_YU}*/};

auto av_to_sdl_pixel_fmt(AVPixelFormat in) {
    for (const auto& [av, sdl] : AV_TO_SDL_PIXEL_FMT_MAP) {
        if (in == av) return sdl;
    }
    return SDL_PIXELFORMAT_UNKNOWN;
}

class Sdl2Init {
    struct Private {};

 public:
    explicit Sdl2Init(Private /*unused*/) {
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
        SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));
    }
    ~Sdl2Init() {
        // LeakSanitizer trips here
#ifndef PLAI_SDL_NO_QUIT
        SDL_Quit();
#endif
    }
    static std::shared_ptr<Sdl2Init> instance() {
        static std::mutex mut{};
        std::unique_lock lk{mut};
        static std::weak_ptr<Sdl2Init> inst{};
        auto ptr = inst.lock();
        if (!ptr) {
            ptr = std::make_shared<Sdl2Init>(Private{});
            inst = ptr;
        }
        return ptr;
    }

 private:
};

Vec<int> scaled_size(Vec<int> win, Vec<int> texture, Vec<double> scaling) {
    auto text_aspect_ratio = double(texture.x) / texture.y;
    auto scaled_win = Vec<double>{scaling.x * win.x, scaling.y * win.y};
    auto win_aspect_ratio = scaled_win.x / scaled_win.y;
    if (text_aspect_ratio >= win_aspect_ratio) {
        // texture is wider -> limited by width
        double scaling = scaled_win.x / texture.x;
        return {.x = int(scaled_win.x), .y = int(scaling * texture.y)};
    } else {
        // texture is higher -> limited by height
        double scaling = scaled_win.y / texture.y;
        return {.x = int(scaling * texture.x), .y = int(scaled_win.y)};
    }
}

int render_dst_start(Position pos, int text_size, int win_size) {
    using enum Position;
    switch (pos) {
        case Begin: return 0;
        case End: return win_size - text_size;
        case Middle: return (win_size - text_size) / 2;
    }
    std::unreachable();
}

SDL_Rect render_dst_scaled(Position vertical, Position horizontal,
                           Vec<int> text, Vec<int> win, Vec<double> scaling) {
    auto scaled = scaled_size(win, text, scaling);

    auto x = render_dst_start(horizontal, scaled.x, win.x);
    auto y = render_dst_start(vertical, scaled.y, win.y);
    return {.x = x, .y = y, .w = scaled.x, .h = scaled.y};
}

SDL_Rect render_dst_stretched(Position vertical, Position horizontal,
                              Vec<int> win, Vec<double> scaling) {
    auto scaled = Vec<int>{
        .x = int(win.x * scaling.x),
        .y = int(win.y * scaling.y),
    };

    auto x = render_dst_start(vertical, scaled.x, win.x);
    auto y = render_dst_start(horizontal, scaled.y, win.y);
    return {.x = x, .y = y, .w = scaled.x, .h = scaled.y};
}

void update_texture_with_av_frame(SDL_Texture* text, const AVFrame* frame,
                                  SDL_PixelFormatEnum sdl_pix_fmt) {
    if (sdl_pix_fmt == SDL_PIXELFORMAT_IYUV) {
        if (frame->linesize[0] > 0 && frame->linesize[1] > 0 &&
            frame->linesize[2] > 0) {
            SDL_UpdateYUVTexture(text, nullptr, frame->data[0],
                                 frame->linesize[0], frame->data[1],
                                 frame->linesize[1], frame->data[2],
                                 frame->linesize[2]);
        } else if (frame->linesize[0] < 0 && frame->linesize[1] < 0 &&
                   frame->linesize[2] < 0) {
            SDL_UpdateYUVTexture(
                text, nullptr,
                frame->data[0] + frame->linesize[0] * (frame->height - 1),
                -frame->linesize[0],
                frame->data[1] +
                    frame->linesize[1] * (AV_CEIL_RSHIFT(frame->height, 1) - 1),
                -frame->linesize[1],
                frame->data[2] +
                    frame->linesize[2] * (AV_CEIL_RSHIFT(frame->height, 1) - 1),
                -frame->linesize[2]);
        } else {
            PLAI_ERR("Mixed negative and positive linesizes are not supported");
        }
    } else {
        if (frame->linesize[0] < 0) {
            SDL_CHECK(SDL_UpdateTexture(
                text, nullptr,
                frame->data[0] + frame->linesize[0] * (frame->height - 1),
                -frame->linesize[0]));
        } else {
            SDL_CHECK(SDL_UpdateTexture(text, nullptr, frame->data[0],
                                        frame->linesize[0]));
        }
    }
}

}  // namespace
}  // namespace detail

class VoidDev final : public Renderer::Device {
 public:
    void set_renderer(Renderer& rend) override { (void)rend; }
    void upload_texture(size_t idx, const media::Frame& frm) override {
        (void)idx;
        (void)frm;
    }
    void swap_textures(size_t idx_a, size_t idx_b) override {
        (void)idx_a;
        (void)idx_b;
    }
    void update() override {}
};

class SdlDev final : public Renderer::Device {
 public:
    void set_renderer(Renderer& rend) override { (void)rend; }
    void upload_texture(size_t idx, const media::Frame& frm) override {
        (void)idx;
        (void)frm;
    }
    void swap_textures(size_t idx_a, size_t idx_b) override {
        (void)idx_a;
        (void)idx_b;
    }
    void update() override {}
};

Renderer make_renderer(size_t texture_count, FrontendType frontend_type) {
    using enum FrontendType;
    switch (frontend_type) {
        case Void: return Renderer(texture_count, std::make_unique<VoidDev>());
        case Sdl2: return Renderer(texture_count, std::make_unique<SdlDev>());
    }
    ::std::unreachable();
}
}  // namespace plai::frontend
