#pragma once

#include <plai/vec.hpp>
#include <variant>

namespace plai {

/**
 * \brief User requests exit
 * */
struct Quit {};

using Event = std::variant<Quit>;

}  // namespace plai
