#pragma once

#include <cstddef>
namespace plai::media {

using hwaccel_t = int;

/**
 * \brief Hardware acceleration
 * */
class HwAccel {
 public:
 private:
};

class HwAccelMeta {
 public:
  constexpr HwAccelMeta() noexcept = default;
  constexpr explicit HwAccelMeta(hwaccel_t type) noexcept : m_type(type) {}

  constexpr hwaccel_t type() const noexcept { return m_type; }
  const char* name() const noexcept;

 private:
  hwaccel_t m_type{};
};

class HwAccelRange {
 public:
  class Iter {
   public:
    using difference_type = std::ptrdiff_t;
    using value_type = HwAccelMeta;
    using reference = HwAccelMeta;
    using pointer_type = value_type*;
    constexpr Iter() noexcept {}

    constexpr explicit Iter(hwaccel_t type) noexcept : m_meta(type) {}

    Iter& operator++() noexcept;

    Iter operator++(int) noexcept {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    reference operator*() noexcept { return m_meta; }
    pointer_type operator->() noexcept { return &m_meta; }

    constexpr bool operator==(const Iter& other) const noexcept {
      return m_meta.type() == other.m_meta.type();
    }

   private:
    // really an enumeration in ffmpeg, but avoid polluting global namespace
    HwAccelMeta m_meta{};
  };

  constexpr explicit HwAccelRange(hwaccel_t first) noexcept : m_first(first) {}

  Iter begin() const noexcept;
  Iter end() const noexcept;

 private:
  hwaccel_t m_first{};
};

HwAccelRange supported_hardware_accelerators() noexcept;

}  // namespace plai::media
