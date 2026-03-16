#pragma once

#include <exec/task.hpp>
#include <plai/ex/scheduler.hpp>
#include <plai/ex/visit_on.hpp>
#include <plai/frac.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <plai/media/frame.hpp>
#include <plai/media2/media.hpp>
#include <plai/ring_buffer.hpp>

namespace plai::media2 {

template <stdexec::scheduler S>
class AsyncDecodingStream {
    static constexpr size_t BUF_SIZE = 8;

 public:
    Frac<int> fps() const noexcept { return m_fps; }
    bool is_still() const noexcept { return m_still; }

    constexpr static auto create(S sched, media2::Media m,
                                 media::HwAccel hwaccel = {}) {
        return stdexec::just() |
               ex::visit_on(sched, [=, m = std::move(m)] mutable {
                   auto dem = media::Demux(m.data());
                   auto [idx, stream] = dem.best_video_stream();
                   auto dec = media::Decoder(stream, hwaccel);

                   return AsyncDecodingStream(
                       std::move(sched), stream.fps(), stream.is_still_image(),
                       std::move(m), std::move(dem), std::move(dec));
               });
    }

    constexpr auto next() {
        return ex::visit_on(m_sched, [&] {
            // TODO: Make unblocking (need a separate type)
            return m_buf.pop();
        });
    }

 private:
    AsyncDecodingStream(S sched, Frac<int> fps, bool still, media2::Media med,
                        media::Demux dem, media::Decoder dec)
        : m_sched(std::move(sched)), m_fps(fps), m_still(still) {}

    exec::task<void> work();

    // TODO: Make this stdexec-friendly
    RingBuffer<media::Frame> m_buf{BUF_SIZE};
    S m_sched;
    Frac<int> m_fps;
    bool m_still;
};

template <stdexec::scheduler S, class... Ts>
constexpr auto decode(S&& s, Ts&&... ts) {
    return AsyncDecodingStream<S>::create(std::forward<S>(s),
                                          std::forward<Ts>(ts)...);
}

}  // namespace plai::media2
