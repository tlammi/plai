#pragma once

#include <plai/vec.hpp>

namespace plai {

template <class T>
struct Rect {
    T x;
    T y;
    T w;
    T h;
};

/**
 * \brief Create largest possible rectangle within a bounding rect
 *
 * Creates the largest possible rectangle placed at the center of bounding
 * rectangle with aspect ratio of dims.
 * */
template <class T>
constexpr Rect<T> inner_centered_rect(Vec<T> dims, Vec<T> bounding) {
    const double aspect_dims = double(dims.x) / dims.y;
    const double aspect_bounding = double(bounding.x) / bounding.y;

    if (aspect_dims >= aspect_bounding) {
        // limited by width
        T w = bounding.x;
        T h = w * double(dims.y) / dims.x;
        T offset = double(bounding.y - h) / 2;
        return {.x = 0, .y = offset, .w = w, .h = h};
    } else {
        // limited by height
        T h = bounding.y;
        T w = h * double(dims.x) / dims.y;
        T offset = double(bounding.x - w) / 2;
        return {.x = offset, .y = 0, .w = w, .h = h};
    }
}

}  // namespace plai
