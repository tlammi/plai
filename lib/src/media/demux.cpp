#include <cassert>
#include <cstring>
#include <plai/logs/logs.hpp>
#include <plai/media/demux.hpp>
#include <plai/util/defer.hpp>
#include <print>

#include "av_check.hpp"
#include "plai/media/exceptions.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

namespace plai::media {
namespace {
namespace demux_detail {

constexpr size_t AV_IO_BUFFER_SIZE = 8 * 1024;
}  // namespace demux_detail
}  // namespace

Demux::Demux() : m_ctx(avformat_alloc_context()) {
  if (!m_ctx) throw std::bad_alloc();
}

Demux::Demux(std::span<const uint8_t> buf)
    : m_buf(buf), m_ctx(avformat_alloc_context()) {
  if (!m_ctx) throw std::bad_alloc();

  // TODO: Leaks memory
  void* iobuf = av_malloc(demux_detail::AV_IO_BUFFER_SIZE);
  if (!iobuf) throw std::bad_alloc();

  m_io_ctx = avio_alloc_context(
      reinterpret_cast<uint8_t*>(iobuf), demux_detail::AV_IO_BUFFER_SIZE, 0,
      this, &Demux::buffer_read, nullptr, &Demux::buffer_seek);
  m_ctx->pb = m_io_ctx;
  // some one made a nice design decision and avformat_free_context() frees
  // the passed context on failure :)
  Defer d{[&] { m_ctx = nullptr; }};
  AV_CHECK(avformat_open_input(&m_ctx, "dummy", nullptr, nullptr));
  d.cancel();
}

Demux::Demux(const stdfs::path& src) : Demux() {
  // some one made a nice design decision and avformat_free_context() frees
  // the passed context on failure :)
  Defer d{[&] { m_ctx = nullptr; }};
  AV_CHECK(avformat_open_input(&m_ctx, src.native().c_str(), nullptr, nullptr));
  d.cancel();
}

Demux::Demux(Demux&& other) noexcept
    : m_buf(std::exchange(other.m_buf, {})),
      m_ctx(std::exchange(other.m_ctx, nullptr)),
      m_io_ctx(std::exchange(other.m_io_ctx, nullptr)) {
  if (m_io_ctx) m_io_ctx->opaque = this;
}

Demux& Demux::operator=(Demux&& other) noexcept {
  auto tmp = Demux(m_ctx);
  m_buf = std::exchange(other.m_buf, {});
  m_ctx = std::exchange(other.m_ctx, nullptr);
  m_io_ctx = std::exchange(other.m_io_ctx, nullptr);
  if (m_io_ctx) m_io_ctx->opaque = this;
  return *this;
}

Demux::~Demux() {
  if (m_ctx) {
    avformat_close_input(&m_ctx);
    avformat_free_context(m_ctx);
  }
  if (m_io_ctx) {
    av_free(m_io_ctx->buffer);
    avio_context_free(&m_io_ctx);
  }
}

bool Demux::operator>>(Packet& pkt) {
  av_packet_unref(pkt.raw());
  int res = av_read_frame(m_ctx, pkt.raw());
  if (res == 0) return true;
  if (res == AVERROR_EOF) return false;
  throw AVException(res);
}

StreamViewSpan Demux::streams() noexcept {
  assert(m_ctx);
  return {m_ctx->streams, m_ctx->nb_streams};
}
std::pair<std::size_t, StreamView> Demux::best_video_stream() {
  assert(m_ctx);
  int res = av_find_best_stream(m_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (res < 0) throw AVException(res);
  return {res, StreamView(m_ctx->streams[res])};
}

int Demux::buffer_read(void* userdata, uint8_t* buf, int buflen) noexcept {
  Demux* self = static_cast<Demux*>(userdata);
  assert(self->m_buf_offset <= self->m_buf.size());
  PLAI_TRACE("size: {}", self->m_buf.size());
  PLAI_TRACE("offset: {}", self->m_buf_offset);
  size_t left_in_buf = self->m_buf.size() - self->m_buf_offset;
  auto count = std::min(buflen, static_cast<int>(left_in_buf));
  std::memcpy(buf, self->m_buf.data(), count);
  self->m_buf_offset += count;
  PLAI_TRACE("read {} bytes", count);
  return count;
}

int64_t Demux::buffer_seek(void* userdata, int64_t offset,
                           int whence) noexcept {
  Demux* self = static_cast<Demux*>(userdata);
  PLAI_TRACE("seek offset: {}", self->m_buf_offset);
  switch (whence) {
    case SEEK_SET:
      if (offset < 0) return -1;
      if (static_cast<size_t>(offset) > self->m_buf.size()) return -1;
      self->m_buf_offset = offset;
      return 0;
    case SEEK_CUR:
      return -1;
      break;
    case SEEK_END:
      if (offset >= 0) return -1;
      if (static_cast<size_t>(-1 * offset) > self->m_buf.size()) return -1;
      self->m_buf_offset += offset;
      return 0;
      break;
    default:
      return -1;
  }
}
}  // namespace plai::media
