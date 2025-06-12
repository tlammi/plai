#pragma once

#include <plai/type_traits.hpp>

namespace plai::st {

template <auto V>
using tag_t = value_pack<V>;

template <auto V>
constexpr tag_t<V> tag{};

}  // namespace plai::st
