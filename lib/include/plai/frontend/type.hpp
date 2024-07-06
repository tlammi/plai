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

constexpr std::string_view frontend_name(FrontendType t) {
  using enum FrontendType;
  switch (t) {
    case Sdl2:
      return "sdl2";
    case Void:
      return "void";
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
