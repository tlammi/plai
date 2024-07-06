#include <plai/frontend/frontend.hpp>

namespace plai {
FrontendBuilder frontend(FrontendType type) {
  return FrontendBuilder().set_window(make_window(type));
  // Pass
}
}  // namespace plai
