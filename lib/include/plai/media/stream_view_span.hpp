#pragma once

#include <cassert>
#include <cstddef>
#include <plai/media/forward.hpp>
#include <plai/media/stream_view.hpp>
#include <span>

namespace plai::media {

class StreamViewSpan {
 public:
  constexpr StreamViewSpan(AVStream** streams, size_t count) noexcept
      : m_streams(streams, count) {}

  StreamView operator[](size_t idx) const noexcept {
    assert(idx < m_streams.size());
    return StreamView(m_streams[idx]);
  }

  constexpr size_t size() const noexcept { return m_streams.size(); }
  constexpr size_t length() const noexcept { return m_streams.size(); }

  /**
   * \brief Return the first video stream in the span
   * */
  [[nodiscard]] std::pair<size_t, StreamView> any_video_stream();

 private:
  std::span<AVStream*> m_streams;
};
}  // namespace plai::media
