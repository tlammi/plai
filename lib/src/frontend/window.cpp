#include <plai/frontend/window.hpp>

#include "sdl2.hpp"

namespace plai {

std::unique_ptr<Window> make_window(FrontendType type) {
  using enum FrontendType;
  switch (type) {
    case Sdl2:
      return make_sdl2_window();
    case Void:
      return nullptr;
  }
  std::unreachable();
}
}  // namespace plai
