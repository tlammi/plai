#pragma once

#include <cstdint>
#include <plai/media/forward.hpp>
#include <span>

namespace plai::media {

class Packet {
 public:
    Packet();
    constexpr explicit Packet(AVPacket* raw) noexcept : m_raw(raw) {}

    Packet(const Packet&) = delete;
    Packet& operator=(const Packet&) = delete;

    Packet(Packet&& other) noexcept;
    Packet& operator=(Packet&& other) noexcept;

    ~Packet();

    size_t stream_index() const noexcept;

    AVPacket* raw() noexcept { return m_raw; }
    const AVPacket* raw() const noexcept { return m_raw; }

    std::span<const uint8_t> data() const noexcept;

    size_t size() const noexcept;

    explicit operator bool() const noexcept { return size(); }

 private:
    AVPacket* m_raw;
};

}  // namespace plai::media
