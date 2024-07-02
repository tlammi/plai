#pragma once

#include <filesystem>
#include <plai/media/forward.hpp>
#include <plai/media/packet.hpp>
#include <plai/media/stream_view_span.hpp>
#include <vector>

namespace plai::media {

namespace stdfs = std::filesystem;

/**
 * \brief Demultiplexer
 *
 * Reads input media to multiple different streams.
 * */
class Demux {
 public:
  Demux();
  constexpr explicit Demux(AVFormatContext* ctx) noexcept : m_ctx(ctx) {}

  explicit Demux(std::span<const uint8_t> buf);
  explicit Demux(const stdfs::path& src);

  Demux(const Demux&) = delete;
  Demux& operator=(const Demux&) = delete;

  Demux(Demux&& other) noexcept;
  Demux& operator=(Demux&& other) noexcept;

  ~Demux();

  /**
   * \brief Read the next packet
   *
   * Packet is a bundle of data read from the media. Use pkt.stream to check
   * into which stream the packet belongs to.
   *
   * \return True on success and false on end-of-stream. If false is returned
   * the resulting packet is empty.
   * */
  bool operator>>(Packet& pkt);

  StreamViewSpan streams() noexcept;

 private:
  /**
   * \brief Callback for FFmpeg when reading from in-memory buffer
   * */
  static int buffer_read(void* userdata, uint8_t* buf, int buflen) noexcept;

  /**
   * \brief Callback for FFmpeg when reading from in-memory buffer
   * */
  static int64_t buffer_seek(void* userdata, int64_t offset,
                             int whence) noexcept;

  // only used if a buffer is passed via constructor
  std::span<const uint8_t> m_buf{};
  size_t m_buf_offset{};
  AVFormatContext* m_ctx;
  AVIOContext* m_io_ctx{};
};
}  // namespace plai::media
