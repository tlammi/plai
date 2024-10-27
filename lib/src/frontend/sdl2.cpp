
#include "sdl2.hpp"

#include <SDL2/SDL.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>

#include <cassert>
#include <mutex>
#include <plai/frontend/exceptions.hpp>
#include <plai/logs/logs.hpp>
#include <plai/rect.hpp>
#include <plai/util/array.hpp>
#include <print>
#include <thread>

#include "SDL_render.h"

namespace plai::sdl {
#if 0
namespace {
namespace sdl_detail {

struct PixelFmtMapEntry {
    AVPixelFormat av;
    SDL_PixelFormatEnum sdl;
};

constexpr auto AV_TO_SDL_PIXEL_FMT_MAP = std::array<PixelFmtMapEntry, 19>{
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
    {AV_PIX_FMT_YUV420P, SDL_PIXELFORMAT_IYUV},
    {AV_PIX_FMT_YUYV422, SDL_PIXELFORMAT_YUY2},
    {AV_PIX_FMT_UYVY422, SDL_PIXELFORMAT_UYVY}};

auto av_to_sdl_pixel_fmt(AVPixelFormat in) {
    for (const auto &[av, sdl] : AV_TO_SDL_PIXEL_FMT_MAP) {
        if (in == av) return sdl;
    }
    return SDL_PIXELFORMAT_UNKNOWN;
}

auto av_pixel_to_sdl_blend_mode(AVPixelFormat in) {
    switch (in) {
        case AV_PIX_FMT_RGB32:
        case AV_PIX_FMT_RGB32_1:
        case AV_PIX_FMT_BGR32:
        case AV_PIX_FMT_BGR32_1: return SDL_BLENDMODE_BLEND;
        default: return SDL_BLENDMODE_NONE;
    }
}

SDL_Rect img_rect(const SDL_Point &win, const SDL_Point &img) {
    auto res =
        inner_centered_rect(Vec<int>(img.x, img.y), Vec<int>(win.x, win.y));
    return {
        .x = res.x,
        .y = res.y,
        .w = res.w,
        .h = res.h,
    };
}

}  // namespace sdl_detail
}  // namespace

class Sdl2Exception : public FrontendException {
 public:
    Sdl2Exception() noexcept : FrontendException(SDL_GetError()) {}
};

namespace {
namespace sdl2_detail {
[[noreturn]] void panic() { throw Sdl2Exception(); }
}  // namespace sdl2_detail
}  // namespace

#define SDL_CHECK(expr)                                    \
    do {                                                   \
        auto sdl_check_internal_res_ = (expr);             \
        if (sdl_check_internal_res_ < 0) std::terminate(); \
    } while (0)

class Sdl2Init {
    struct Private {};

 public:
    explicit Sdl2Init(Private /*unused*/) {
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

class Sdl2Window final : public Window {
 public:
    Sdl2Window()
        : m_win(SDL_CreateWindow("plai", SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, 600, 400,
                                 SDL_WINDOW_SHOWN)),
          m_rend(SDL_CreateRenderer(
              m_win, -1,
              SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)) {
        if (!m_win) sdl2_detail::panic();
        if (!m_rend) {
            SDL_DestroyWindow(m_win);
            sdl2_detail::panic();
        }
    }

    ~Sdl2Window() {
        PLAI_TRACE("destroying window");
        if (m_win) SDL_DestroyWindow(m_win);
    }

 private:
    void set_event_handlers_impl(FrontendEvents e) override {}

    bool set_fullscreen_impl(bool v) override {
        assert(m_win);
        int res = SDL_SetWindowFullscreen(m_win, v ? SDL_WINDOW_FULLSCREEN : 0);
        return res == 0;
    }

    bool fullscreen_impl() const noexcept override {
        assert(m_win);
        auto flags = SDL_GetWindowFlags(m_win);
        return flags & SDL_WINDOW_FULLSCREEN;
    }

    void render_impl(const media::Frame &frm) override {
        const AVFrame *avframe = frm.raw();
        auto sdl_pix_fmt = sdl_detail::av_to_sdl_pixel_fmt(
            static_cast<AVPixelFormat>(avframe->format));
        if (sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN) {
            sdl_pix_fmt = SDL_PIXELFORMAT_ARGB8888;
        }
        std::println("pixel format: {}", SDL_GetPixelFormatName(sdl_pix_fmt));
        auto *texture =
            SDL_CreateTexture(m_rend, sdl_pix_fmt, SDL_TEXTUREACCESS_STREAMING,
                              frm.width(), frm.height());
        assert(texture);
        SDL_SetTextureBlendMode(
            texture, sdl_detail::av_pixel_to_sdl_blend_mode(
                         static_cast<AVPixelFormat>(avframe->format)));
        // void *pixels{};
        // int pitch{};
        // SDL_LockTexture(texture, nullptr, &pixels, &pitch);
        // memset(pixels, 0, pitch * frm.height());
        // SDL_UnlockTexture(texture);
        if (sdl_pix_fmt == SDL_PIXELFORMAT_IYUV) {
            if (avframe->linesize[0] > 0 && avframe->linesize[1] > 0 &&
                avframe->linesize[2] > 0) {
                SDL_UpdateYUVTexture(texture, nullptr, avframe->data[0],
                                     avframe->linesize[0], avframe->data[1],
                                     avframe->linesize[1], avframe->data[2],
                                     avframe->linesize[2]);
            } else if (avframe->linesize[0] < 0 && avframe->linesize[1] < 0 &&
                       avframe->linesize[2] < 0) {
                SDL_UpdateYUVTexture(
                    texture, nullptr,
                    avframe->data[0] +
                        avframe->linesize[0] * (avframe->height - 1),
                    -avframe->linesize[0],
                    avframe->data[1] +
                        avframe->linesize[1] *
                            (AV_CEIL_RSHIFT(avframe->height, 1) - 1),
                    -avframe->linesize[1],
                    avframe->data[2] +
                        avframe->linesize[2] *
                            (AV_CEIL_RSHIFT(avframe->height, 1) - 1),
                    -avframe->linesize[2]);
            } else {
                PLAI_ERR(
                    "Mixed negative and positive linesizes are not supported");
            }
        } else {
            if (avframe->linesize[0] < 0) {
                int res = SDL_UpdateTexture(
                    texture, nullptr,
                    avframe->data[0] +
                        avframe->linesize[0] * (avframe->height - 1),
                    -avframe->linesize[0]);
                if (res) std::println("failed to update texture 1");
            } else {
                int res = SDL_UpdateTexture(texture, nullptr, avframe->data[0],
                                            avframe->linesize[0]);
                if (res) std::println("failed to update texture 2");
            }
        }
        auto d = frm.dims();
        std::println("{}, {}", d.x, d.y);
        auto rect = sdl_detail::img_rect({600, 400}, {d.x, d.y});
        SDL_RenderCopy(m_rend, texture, nullptr, &rect);
        SDL_RenderPresent(m_rend);
    }

    std::shared_ptr<Sdl2Init> m_init_handle = Sdl2Init::instance();
    SDL_Window *m_win{};
    SDL_Renderer *m_rend{};
};

std::unique_ptr<Window> make_sdl2_window() {
    return std::make_unique<Sdl2Window>();
}

#endif

namespace detail {

#define SDL_CHECK(expr) \
    do { detail::sdl_check(expr); } while (0)

struct PixelFmtMapEntry {
    AVPixelFormat av;
    SDL_PixelFormatEnum sdl;
};
constexpr auto AV_TO_SDL_PIXEL_FMT_MAP = std::array<PixelFmtMapEntry, 19>{
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
    {AV_PIX_FMT_YUV420P, SDL_PIXELFORMAT_IYUV},
    {AV_PIX_FMT_YUYV422, SDL_PIXELFORMAT_YUY2},
    {AV_PIX_FMT_UYVY422, SDL_PIXELFORMAT_UYVY}};

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
}  // namespace detail

class SdlTexture final : public Texture {
 public:
    explicit SdlTexture(UniqPtr<SDL_Texture> text, SDL_Renderer* rend,
                        TextureAccess access)
        : m_text(std::move(text)), m_rend(rend) {
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
    void render_frame(const media::Frame& frame) final {
        const AVFrame* avframe = frame.raw();
        auto sdl_pix_fmt = detail::av_to_sdl_pixel_fmt(
            static_cast<AVPixelFormat>(avframe->format));
        if (sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN) {
            sdl_pix_fmt = SDL_PIXELFORMAT_ARGB8888;
        }
        // void *pixels{};
        // int pitch{};
        // SDL_LockTexture(texture, nullptr, &pixels, &pitch);
        // memset(pixels, 0, pitch * frm.height());
        // SDL_UnlockTexture(texture);
        if (sdl_pix_fmt == SDL_PIXELFORMAT_IYUV) {
            if (avframe->linesize[0] > 0 && avframe->linesize[1] > 0 &&
                avframe->linesize[2] > 0) {
                SDL_UpdateYUVTexture(m_text.get(), nullptr, avframe->data[0],
                                     avframe->linesize[0], avframe->data[1],
                                     avframe->linesize[1], avframe->data[2],
                                     avframe->linesize[2]);
            } else if (avframe->linesize[0] < 0 && avframe->linesize[1] < 0 &&
                       avframe->linesize[2] < 0) {
                SDL_UpdateYUVTexture(
                    m_text.get(), nullptr,
                    avframe->data[0] +
                        avframe->linesize[0] * (avframe->height - 1),
                    -avframe->linesize[0],
                    avframe->data[1] +
                        avframe->linesize[1] *
                            (AV_CEIL_RSHIFT(avframe->height, 1) - 1),
                    -avframe->linesize[1],
                    avframe->data[2] +
                        avframe->linesize[2] *
                            (AV_CEIL_RSHIFT(avframe->height, 1) - 1),
                    -avframe->linesize[2]);
            } else {
                PLAI_ERR(
                    "Mixed negative and positive linesizes are not supported");
            }
        } else {
            if (avframe->linesize[0] < 0) {
                int res = SDL_UpdateTexture(
                    m_text.get(), nullptr,
                    avframe->data[0] +
                        avframe->linesize[0] * (avframe->height - 1),
                    -avframe->linesize[0]);
                if (res) std::println("failed to update texture 1");
            } else {
                int res =
                    SDL_UpdateTexture(m_text.get(), nullptr, avframe->data[0],
                                      avframe->linesize[0]);
                if (res) std::println("failed to update texture 2");
            }
        }
    }

    SDL_Texture* raw() const noexcept { return m_text.get(); }

 private:
    UniqPtr<SDL_Texture> m_text;
    SDL_Renderer* m_rend;
};

class SdlFrontend final : public Frontend {
 public:
    std::unique_ptr<Texture> texture(
        std::span<RenderTargetOpts> target_opts) final {
        auto res = std::make_unique<SdlTexture>(
            make_texture(m_rend.get(), TextureAccess::Streaming), m_rend.get(),
            TextureAccess::Streaming);
        m_text = res->raw();  // TODO: remove this
        return res;
    }

    void render_current() final {
        SDL_RenderCopy(m_rend.get(), m_text, nullptr, nullptr);
        SDL_RenderPresent(m_rend.get());
    }

 private:
    std::shared_ptr<detail::Sdl2Init> m_sdl{detail::Sdl2Init::instance()};
    UniqPtr<SDL_Window> m_win{sdl::make_window("plai")};
    UniqPtr<SDL_Renderer> m_rend{sdl::make_renderer(m_win.get())};
    SDL_Texture* m_text{};
};

std::unique_ptr<Frontend> sdl_frontend() {
    return std::make_unique<SdlFrontend>();
}

}  // namespace plai::sdl
