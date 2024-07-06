#pragma once

#include <plai/frontend/events.hpp>

namespace plai {

/**
 * \brief Options for frontends
 * */
struct FrontendOptions {
  FrontendEvents events{};
};
}  // namespace plai
