
#include "sdl2.hpp"

#include <SDL2/SDL.h>

#include <cassert>
#include <mutex>
#include <plai/frontend/exceptions.hpp>
#include <plai/logs/logs.hpp>

namespace plai {

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
                               SDL_WINDOW_SHOWN)) {
    if (!m_win) sdl2_detail::panic();
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

  std::shared_ptr<Sdl2Init> m_init_handle = Sdl2Init::instance();
  SDL_Window* m_win{};
};

std::unique_ptr<Window> make_sdl2_window() {
  return std::make_unique<Sdl2Window>();
}
}  // namespace plai
