#pragma once

#include <stdexcept>

namespace plai::media {
class AVException : public std::runtime_error {
 public:
  explicit AVException(int ec) noexcept;
  explicit AVException(const char* msg) noexcept : std::runtime_error(msg) {}
};
}  // namespace plai::media
