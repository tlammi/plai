#include <plai/app.hpp>
#include <plai/mat.hpp>
#include <print>

int main() {
  plai::AppBuilder b{};
  b.frontend(plai::make_frontend("opencv"));
  auto app = b.commit();
  return app.run();
}

