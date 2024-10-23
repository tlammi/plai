
#include "sdl2.hpp"

#include <SDL2/SDL.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>

#include <cassert>
#include <mutex>
#include <plai/frontend/exceptions.hpp>
#include <plai/logs/logs.hpp>
#include <plai/util/array.hpp>
#include <print>

#include "SDL_pixels.h"
#include "SDL_render.h"

namespace plai {
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
  for (const auto& [av, sdl] : AV_TO_SDL_PIXEL_FMT_MAP) {
    if (in == av) return sdl;
  }
  return SDL_PIXELFORMAT_UNKNOWN;
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

#define SDL_CHECK(expr)                                \
  do {                                                 \
    auto sdl_check_internal_res_ = (expr);             \
    if (sdl_check_internal_res_ < 0) std::terminate(); \
  } while (0)

class Sdl2Init {
  struct Private {};

 public:
  explicit Sdl2Init(Private /*unused*/) { SDL_CHECK(SDL_Init(SDL_INIT_VIDEO)); }
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
        m_rend(SDL_CreateRenderer(m_win, -1, SDL_RENDERER_ACCELERATED)) {
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

  void render_impl(const media::Frame& frm) override {
    const AVFrame* avframe = frm.raw();
    auto sdl_pix_fmt = sdl_detail::av_to_sdl_pixel_fmt(
        static_cast<AVPixelFormat>(avframe->format));
    if (sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN)
      sdl_pix_fmt = SDL_PIXELFORMAT_ARGB8888;
    auto* texture =
        SDL_CreateTexture(m_rend, sdl_pix_fmt, SDL_TEXTUREACCESS_STREAMING,
                          frm.width(), frm.height());
    assert(texture);
    if (avframe->linesize[0] < 0) {
      int res = SDL_UpdateTexture(
          texture, nullptr,
          avframe->data[0] + avframe->linesize[0] * (avframe->height - 1),
          -avframe->linesize[0]);
      if (res) std::println("failed to update texture 1");
    } else {
      int res = SDL_UpdateTexture(texture, nullptr, avframe->data[0],
                                  avframe->linesize[0]);
      if (res) std::println("failed to update texture 2");
    }
    SDL_RenderCopy(m_rend, texture, nullptr, nullptr);
    SDL_RenderPresent(m_rend);
    // SDL_DestroyTexture(texture);
  }

  std::shared_ptr<Sdl2Init> m_init_handle = Sdl2Init::instance();
  SDL_Window* m_win{};
  SDL_Renderer* m_rend{};
};

std::unique_ptr<Window> make_sdl2_window() {
  return std::make_unique<Sdl2Window>();
}
}  // namespace plai
