#pragma once

#include <filesystem>
#include <plai/media/forward.hpp>
#include <plai/media/frame.hpp>
#include <plai/media/hwaccel.hpp>
#include <plai/media/packet.hpp>
#include <plai/media/stream_view.hpp>

namespace plai::media {

class Decoder {
 public:
    Decoder();
    explicit Decoder(const HwAccel& hw_accel);

    explicit Decoder(StreamView str);
    Decoder(StreamView str, const HwAccel& hw_accel);

    // TODO: add constructors for setting codec explicitly

    constexpr explicit Decoder(AVCodecContext* ctx) noexcept : m_ctx(ctx) {}

    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    Decoder(Decoder&& other) noexcept;
    Decoder& operator=(Decoder&& other) noexcept;

    ~Decoder();
    /**
     * \brief Send packet to the decoder
     *
     * The decoder might require one or more packets
     * to produce a single frame.
     * */
    Decoder& operator<<(const Packet& pkt);

    /**
     * \brief Extract a frame from the decoder
     *
     *
     * Extracts the next frame from the decoder. End of stream is signaled by
     * empty frame, i.e. return value is true and bool(frm) == false;
     *
     * \return True if a frame could be extracted, false if not. If the
     * resulting frame is empty the decoder was fully flushed and no more data
     * is available. Calls to the operator aftewards result in exceptions.
     * */
    bool operator>>(Frame& frm);

    int width() const noexcept;
    int height() const noexcept;
    Vec<int> dims() const noexcept { return {width(), height()}; }

 private:
    AVCodecContext* m_ctx;
#ifndef NDEBUG
    // used for verifying that all packets come from a single stream
    std::size_t m_stream_idx{std::numeric_limits<std::size_t>::max()};
#endif
};

}  // namespace plai::media
