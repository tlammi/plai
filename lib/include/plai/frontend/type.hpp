#pragma once

#include <format>
#include <plai/exceptions.hpp>
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
/**
 * \brief Scale texture and keep the aspect ratio
 *
 * Scales the image up or down so one of the dimensions matches the given upper
 * limit and the other one is less than the limit.
 * */
struct ScaleFit {
    double max_x = 100.0;  ///< max horizontal size as percent of window width
    double max_y = 100.0;  ///< max vertical size as percent of window height
};

/**
 * \brief Scale the image so it fills the given area
 *
 * This deforms the input image so x and y dimensions match the wanted.
 * */
struct ScaleStretch {
    double x = 100.0;  ///< horizontal size as percent of window width
    double y = 100.0;  ///< vertical size as percent of window height
};

using Scaling = std::variant<ScaleFit, ScaleStretch>;

struct RenderTargetOpts {
    Scaling scaling{};  ///< How to scale the texture for rendering

    /// Where to place the image horizontally
    Position pos_x = Position::Middle;

    /// Where to place the image vertically
    Position pos_y = Position::Middle;
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
