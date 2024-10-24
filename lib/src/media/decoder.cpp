#include <plai/exceptions.hpp>
#include <plai/media/decoder.hpp>
#include <plai/util/defer.hpp>

#include "av_check.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace plai::media {

Decoder::Decoder() : m_ctx(avcodec_alloc_context3(nullptr)) {
  if (!m_ctx) throw std::bad_alloc();
}

Decoder::Decoder(StreamView str) : Decoder() {
  AV_CHECK(avcodec_parameters_to_context(m_ctx, str.raw()->codecpar));
  m_ctx->pkt_timebase = str.raw()->time_base;
  auto* codec = avcodec_find_decoder(m_ctx->codec_id);
  if (!codec) throw ValueError("Codec not found");
  // TODO: codec options
  AV_CHECK(avcodec_open2(m_ctx, codec, nullptr));
}

Decoder::Decoder(Decoder&& other) noexcept
    : m_ctx(std::exchange(other.m_ctx, nullptr)) {}

Decoder& Decoder::operator=(Decoder&& other) noexcept {
  auto tmp = Decoder(m_ctx);
  m_ctx = std::exchange(other.m_ctx, nullptr);
  return *this;
}

Decoder::~Decoder() {
  if (m_ctx) avcodec_free_context(&m_ctx);
}
Decoder& Decoder::operator<<(const Packet& pkt) {
  AV_CHECK(avcodec_send_packet(m_ctx, pkt.raw()));
  return *this;
}

bool Decoder::operator>>(Frame& frm) {
  int res = avcodec_receive_frame(m_ctx, frm.raw());
  if (res >= 0) return true;
  if (res == AVERROR(EAGAIN)) return false;
  throw AVException(res);
}
int Decoder::width() const noexcept { return m_ctx->width; }
int Decoder::height() const noexcept { return m_ctx->height; }
}  // namespace plai::media
