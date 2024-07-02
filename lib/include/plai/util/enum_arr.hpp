#pragma once

#include <array>
#include <plai/concepts.hpp>
#include <plai/thirdparty/magic_enum.hpp>
#include <plai/util/cast.hpp>

namespace plai {

template <concepts::enum_type E, class T>
class EnumArr {
 public:
  static constexpr size_t enum_count = magic_enum::enum_count<E>();

  template <std::convertible_to<T>... Ts>
  explicit EnumArr(std::in_place_t /*unused*/, Ts&&... ts)
      : m_arr{std::forward<Ts>(ts)...} {}

  constexpr size_t size() const noexcept { return m_arr.size(); }
  constexpr size_t length() const noexcept { return m_arr.size(); }

  T& operator[](E e) noexcept { return m_arr[underlying_cast(e)]; }
  const T& operator[](E e) const noexcept { return m_arr[underlying_cast(e)]; }

 private:
  std::array<T, enum_count> m_arr{};
};

}  // namespace plai
