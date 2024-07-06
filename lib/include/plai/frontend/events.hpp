#pragma once

#include <cstdint>
#include <functional>
#include <plai/vec.hpp>

namespace plai {

/**
 * \brief Events from frontend
 * */
struct FrontendEvents {
  /// window resize
  std::function<void(plai::Vec<int32_t>)> on_resize{};
};

}  // namespace plai
