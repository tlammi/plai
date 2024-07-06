#include <plai/frontend/type.hpp>
#include <gtest/gtest.h>


TEST(Type, FromStr) {
  auto t = plai::frontend_type("sdl2");
  ASSERT_EQ(t, plai::FrontendType::Sdl2);
}
