
#include "sdl2.hpp"

#include <SDL2/SDL.h>
#include <libavutil/pixfmt.h>

#include <cassert>
#include <mutex>
#include <plai/frontend/exceptions.hpp>
#include <plai/logs/logs.hpp>
#include <plai/rect.hpp>
#include <plai/util/array.hpp>
#include <utility>

#include "SDL_hints.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_video.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

namespace plai::sdl {
namespace detail {
namespace {

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

class SdlTexture final : public Texture {
 public:
    explicit SdlTexture(UniqPtr<SDL_Texture> text, SDL_Renderer* rend,
                        SDL_Window* win, TextureAccess access,
                        SDL_PixelFormatEnum pix_fmt)
        : m_text(std::move(text)),
          m_rend(rend),
          m_win(win),
          m_pix_fmt(pix_fmt) {
        if (access == TextureAccess::Streaming) {
            [[maybe_unused]] void* pixels{};
            [[maybe_unused]] int pitch{};
            SDL_LockTexture(m_text.get(), nullptr, &pixels, &pitch);
        }
    }
    void blend_mode(BlendMode mode) final {
        SDL_CHECK(SDL_SetTextureBlendMode(m_text.get(),
                                          sdl::convert_blend_mode(mode)));
    }
    void alpha(uint8_t alpha) final {
        SDL_CHECK(SDL_SetTextureAlphaMod(m_text.get(), alpha));
    }
    void render_rgb(uint8_t r, uint8_t g, uint8_t b) final {
        SDL_CHECK(SDL_SetRenderDrawColor(m_rend, r, g, b, 0));
        SDL_CHECK(SDL_SetRenderTarget(m_rend, m_text.get()));
        SDL_CHECK(SDL_RenderClear(m_rend));
        SDL_CHECK(SDL_SetRenderTarget(m_rend, nullptr));
    }
    void update(const media::Frame& frame) final {
        m_dims = frame.dims();
        const AVFrame* avframe = frame.raw();
        auto sdl_pix_fmt = detail::av_to_sdl_pixel_fmt(
            static_cast<AVPixelFormat>(avframe->format));
        if (sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN) {
            sdl_pix_fmt = SDL_PIXELFORMAT_ARGB8888;
        }
        if (sdl_pix_fmt != m_pix_fmt) {
            m_text = make_texture(m_rend, plai::TextureAccess::Streaming,
                                  m_dims.x, m_dims.y, sdl_pix_fmt);
            m_pix_fmt = sdl_pix_fmt;
        }
        detail::update_texture_with_av_frame(m_text.get(), avframe,
                                             sdl_pix_fmt);
    }

    void render_to(const RenderTarget& tgt) override {
        Vec<int> win_dims{};
        SDL_GetWindowSize(m_win, &win_dims.x, &win_dims.y);
        Vec<double> scaling{tgt.w, tgt.h};
        if (tgt.scaling == Scaling::Fit) {
            auto dst = detail::render_dst_scaled(tgt.vertical, tgt.horizontal,
                                                 m_dims, win_dims, scaling);
            const SDL_Rect src{.x = 0, .y = 0, .w = m_dims.x, .h = m_dims.y};
            SDL_RenderCopy(m_rend, m_text.get(), &src, &dst);
        } else {
            auto dst = detail::render_dst_stretched(
                tgt.vertical, tgt.horizontal, win_dims, scaling);
            SDL_RenderCopy(m_rend, m_text.get(), nullptr, &dst);
        }
    }

    SDL_Texture* raw() const noexcept { return m_text.get(); }

 private:
    UniqPtr<SDL_Texture> m_text;
    SDL_Renderer* m_rend;
    SDL_Window* m_win;
    SDL_PixelFormatEnum m_pix_fmt;
    Vec<int> m_dims{};
};

class SdlFrontend final : public Frontend {
 public:
    std::optional<Event> poll_event() override {
        SDL_Event event{};
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: return Quit{};
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_SPACE: toggle_fullscreen();
                        default: break;
                    }
                default: break;
            }
        }
        return std::nullopt;
    }

    void stop() override {
        auto evt = SDL_Event{.type = SDL_QUIT};
        SDL_PushEvent(&evt);
    }

    Vec<int> dimensions() override {
        Vec<int> out{};
        SDL_GetWindowSize(m_win.get(), &out.x, &out.y);
        return out;
    }

    void render_clear() final { SDL_RenderClear(m_rend.get()); }

    std::unique_ptr<Texture> texture() final {
        auto res = std::make_unique<SdlTexture>(
            make_texture(m_rend.get(), TextureAccess::Streaming, 1920, 1080,
                         SDL_PIXELFORMAT_IYUV),
            m_rend.get(), m_win.get(), TextureAccess::Streaming,
            SDL_PIXELFORMAT_IYUV);
        m_text = res->raw();  // TODO: remove this
        return res;
    }

    void render_current() final { SDL_RenderPresent(m_rend.get()); }

 private:
    void set_fullscreen_impl(bool v) override {
        m_full_screen = v;
        SDL_SetWindowFullscreen(
            m_win.get(), m_full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    }

    void toggle_fullscreen() { set_fullscreen_impl(!m_full_screen); }

    std::shared_ptr<detail::Sdl2Init> m_sdl{detail::Sdl2Init::instance()};
    UniqPtr<SDL_Window> m_win{sdl::make_window("plai")};
    UniqPtr<SDL_Renderer> m_rend{sdl::make_renderer(m_win.get())};
    SDL_Texture* m_text{};
    bool m_full_screen{false};
};

std::unique_ptr<Frontend> sdl_frontend() {
    return std::make_unique<SdlFrontend>();
}

UniqPtr<SDL_Texture> make_texture(SDL_Renderer* rend, TextureAccess access,
                                  int w, int h, SDL_PixelFormatEnum fmt) {
    return detail::uniq_with_deleter(
        SDL_CreateTexture(rend, fmt, convert_texture_access(access), w, h),
        [](void* t) { SDL_DestroyTexture(static_cast<SDL_Texture*>(t)); });
}

}  // namespace plai::sdl
