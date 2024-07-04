#include <plai/media/hwaccel.hpp>
#include <print>
#include <ranges>
#include <vector>

int main() {
  auto range = plai::media::supported_hardware_accelerators();
  auto accelerators = std::vector(range.begin(), range.end());

  std::println("found {} accelerators", accelerators.size());

  for (auto [i, a] : std::ranges::views::enumerate(accelerators)) {
    std::println("  {}: {}", i + 1, a.name());
  }
}
