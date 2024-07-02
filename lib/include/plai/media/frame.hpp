#pragma once

#include <plai/media/forward.hpp>
#include <plai/vec.hpp>

namespace plai::media {
class Frame {
 public:
  Frame();
  constexpr explicit Frame(AVFrame* raw) noexcept : m_raw(raw) {}

  Frame(const Frame&) = delete;
  Frame& operator=(const Frame&) = delete;

  Frame(Frame&& other) noexcept;
  Frame& operator=(Frame&& other) noexcept;

  ~Frame();

  [[nodiscard]] AVFrame* raw() noexcept { return m_raw; }

  /**
   * \brief False for empty frames
   * */
  explicit operator bool() const noexcept;

  int width() const noexcept;
  int height() const noexcept;
  Vec<int> dims() const noexcept { return {width(), height()}; }

 private:
  AVFrame* m_raw;
};
}  // namespace plai::media
