#pragma once
#include <plai/concepts.hpp>

namespace plai {

template <concepts::enum_type E>
constexpr auto underlying_cast(E e) {
  return static_cast<std::underlying_type_t<E>>(e);
}

template <concepts::enum_type E>
constexpr auto underlying_cast(std::underlying_type_t<E> i) {
  return static_cast<E>(i);
}
}  // namespace plai
