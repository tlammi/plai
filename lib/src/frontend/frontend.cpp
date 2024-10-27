#include <plai/frontend/frontend.hpp>
#include <utility>

#include "sdl2.hpp"

namespace plai {
std::unique_ptr<Frontend> frontend(FrontendType type) {
    using enum FrontendType;
    switch (type) {
        case Sdl2: return sdl::sdl_frontend();
        case Void: return nullptr;
    }
    std::unreachable();
}
}  // namespace plai
