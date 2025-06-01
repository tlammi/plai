#pragma once

#ifndef PLAI_USE_STDFORMAT
#define PLAI_USE_STDFORMAT 1
#endif

#if PLAI_USE_STDFORMAT == 1

#include <format>
#include <print>
namespace plai {

template <class... Ts>
std::string format(std::format_string<Ts...> fmt, Ts&&... ts) {
    return std::format(fmt, std::forward<Ts>(ts)...);
}

template <class... Ts>
void println(std::format_string<Ts...> fmt, Ts&&... ts) {
    return std::println(fmt, std::forward<Ts>(ts)...);
}

template <class... Ts>
void println(std::FILE* stream, std::format_string<Ts...> fmt, Ts&&... ts) {
    return std::println(stream, fmt, std::forward<Ts>(ts)...);
}

}  // namespace plai
#else

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace plai {
template <class... Ts>
std::string format(fmt::format_string<Ts...> f, Ts&&... ts) {
    return fmt::format(f, std::forward<Ts>(ts)...);
}

template <class... Ts>
void println(fmt::format_string<Ts...> f, Ts&&... ts) {
    fmt::print(f, std::forward<Ts>(ts)...);
    fmt::print("\n");
}

template <class... Ts>
void println(std::FILE* stream, fmt::format_string<Ts...> f, Ts&&... ts) {
    fmt::print(stream, f, std::forward<Ts>(ts)...);
    fmt::print("\n");
}

}  // namespace plai
#endif
