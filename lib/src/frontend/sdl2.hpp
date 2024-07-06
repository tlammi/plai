#pragma once

#include <memory>
#include <plai/frontend/window.hpp>

struct SDL_Window;

namespace plai {

class Sdl2Window;

std::unique_ptr<Window> make_sdl2_window();

}  // namespace plai
