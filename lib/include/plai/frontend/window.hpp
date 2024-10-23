#pragma once

#include <memory>
#include <plai/frontend/events.hpp>
#include <plai/frontend/type.hpp>
#include <plai/media/frame.hpp>

namespace plai {

class Window {
 public:
  virtual ~Window() = default;

  void set_event_handlers(FrontendEvents e) {
    set_event_handlers_impl(std::move(e));
  }
  bool set_fullscreen(bool value = true) { return set_fullscreen_impl(value); }
  [[nodiscard]] bool fullscreen() const noexcept { return fullscreen_impl(); }
  void render(const media::Frame& frm) { render_impl(frm); }

 private:
  virtual void set_event_handlers_impl(FrontendEvents e) = 0;
  virtual bool set_fullscreen_impl(bool v) = 0;
  virtual bool fullscreen_impl() const noexcept = 0;
  virtual void render_impl(const plai::media::Frame& frm) = 0;
};

std::unique_ptr<Window> make_window(FrontendType type);

}  // namespace plai
