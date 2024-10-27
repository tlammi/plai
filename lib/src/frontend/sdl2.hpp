#pragma once

#include <SDL2/SDL.h>

#include <memory>
#include <plai/frontend/exceptions.hpp>
#include <plai/frontend/frontend.hpp>
#include <plai/frontend/type.hpp>
#include <plai/frontend/window.hpp>

namespace plai::sdl {
template <class T>
using UniqPtr = std::unique_ptr<T, void (*)(void*)>;
namespace detail {

inline void sdl_check(int res) {
    if (res < 0) { throw FrontendException(SDL_GetError()); }
}

template <class T>
auto uniq_with_deleter(T* ptr, void (*deleter)(void*)) {
    return UniqPtr<T>(ptr, deleter);
}

}  // namespace detail

constexpr auto convert_texture_access(TextureAccess access) {
    using enum TextureAccess;
    switch (access) {
        case Static: return SDL_TEXTUREACCESS_STATIC;
        case Streaming: return SDL_TEXTUREACCESS_STREAMING;
    }
    std::unreachable();
}

constexpr SDL_BlendMode convert_blend_mode(BlendMode mode) {
    using enum BlendMode;
    switch (mode) {
        case None: return SDL_BLENDMODE_NONE;
        case Blend: return SDL_BLENDMODE_BLEND;
    }
    std::unreachable();
}

inline auto make_window(const char* title, int w = 600, int h = 400) {
    return detail::uniq_with_deleter(
        SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, w, h, 0),
        [](void* win) { SDL_DestroyWindow(static_cast<SDL_Window*>(win)); });
}

inline auto make_renderer(SDL_Window* win) {
    return detail::uniq_with_deleter(
        SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED),
        [](void* r) { SDL_DestroyRenderer(static_cast<SDL_Renderer*>(r)); });
}

inline auto make_texture(SDL_Renderer* rend, TextureAccess access, int w = 600,
                         int h = 400) {
    return detail::uniq_with_deleter(
        SDL_CreateTexture(rend, SDL_PIXELFORMAT_IYUV,
                          convert_texture_access(access), w, h),
        [](void* t) { SDL_DestroyTexture(static_cast<SDL_Texture*>(t)); });
}

std::unique_ptr<Frontend> sdl_frontend();

}  // namespace plai::sdl

