#pragma once

#include <plai/exceptions.hpp>
#include <plai/format.hpp>
#include <ranges>
#include <string_view>
#include <utility>

namespace plai {

enum class FrontendType {
    Sdl2,  ///< SDL2 frontend
    Void,  ///< frontend discarding the frames
};

enum class BlendMode {
    None,   // draw over
    Blend,  // alpha blendingrgb
            // rgb = (src * alpha) + (dst * (1-alpha))
            // dst_alpha = alpha + (dst_alpha * (1-alpha))
};

/**
 * \brief Position of a resource
 * */
enum class Position {
    Begin,   ///< Position the resource at the start of some range
    Middle,  ///< Position the resource at the middle of some range
    End,     ///< Position the resource at the end of some range
};

/**
 * \brief How the texture is to be used
 *
 * */
enum class TextureAccess {
    /**
     * \brief Changes rarely
     *
     * Use this e.g. for backgrounds
     * */
    Static,
    /**
     * \brief Changes often
     *
     * Use this when updating the view frequently.
     * */
    Streaming,
};

enum class Scaling {
    Fit,      // Scale image without distortion
    Stretch,  // Force fit the target rectangle
};

struct RenderTarget {
    double w = 1.0;
    double h = 1.0;
    Scaling scaling = Scaling::Fit;
    Position vertical = Position::Begin;
    Position horizontal = Position::Begin;
};

constexpr std::string_view frontend_name(FrontendType t) {
    using enum FrontendType;
    switch (t) {
        case Sdl2: return "sdl2";
        case Void: return "void";
    }
    std::unreachable();
}

constexpr FrontendType frontend_type(std::string_view s) {
    namespace rv = std::ranges::views;
    namespace r = std::ranges;
    using enum FrontendType;
    static constexpr auto to_lower = [](char c) {
        if (c >= 'A' && c <= 'Z')
            return static_cast<char>(static_cast<char>(c - 'A') + 'a');
        return c;
    };
    auto lower = rv::transform(s, to_lower);
    using namespace std::literals::string_view_literals;
    if (r::equal(lower, "sdl2"sv)) return Sdl2;
    if (r::equal(lower, "void"sv)) return Void;
    throw ValueError(std::format("unknown frontend: {}", s));
}
}  // namespace plai
