#include <plai/exceptions.hpp>
#include <plai/media/stream_view_span.hpp>

namespace plai::media {

std::pair<size_t, StreamView> StreamViewSpan::any_video_stream() {
  for (size_t i = 0; i < m_streams.size(); ++i) {
    auto s = StreamView(m_streams[i]);
    if (s.video()) return {i, s};
  }
  throw ValueError("no video streams");
}
}  // namespace plai::media
