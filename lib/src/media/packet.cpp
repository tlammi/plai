#include <cassert>
#include <plai/media/packet.hpp>
#include <utility>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace plai::media {

Packet::Packet() : m_raw(av_packet_alloc()) {
    if (!m_raw) throw std::bad_alloc();
}

Packet::Packet(Packet&& other) noexcept
    : m_raw(std::exchange(other.m_raw, nullptr)) {}

Packet& Packet::operator=(Packet&& other) noexcept {
    auto tmp = Packet(std::move(other));
    std::swap(m_raw, tmp.m_raw);
    return *this;
}

Packet::~Packet() {
    if (m_raw) av_packet_free(&m_raw);
}
std::span<const uint8_t> Packet::data() const noexcept {
    return {m_raw->data, static_cast<size_t>(m_raw->size)};
}
size_t Packet::size() const noexcept {
    return static_cast<size_t>(m_raw->size);
}

size_t Packet::stream_index() const noexcept {
    assert(m_raw->stream_index >= 0);
    return m_raw->stream_index;
}

}  // namespace plai::media
