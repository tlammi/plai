#pragma once

#include <plai/frac.hpp>
#include <plai/media/frame.hpp>
#include <plai/persist_buffer.hpp>
#include <plai/ring_buffer.hpp>
namespace plai::media {

class DecodingStream {
    friend class DecodingPipeline;

 public:
    static constexpr auto unknown = std::numeric_limits<size_t>::max();
    Frac<int> fps() const noexcept { return m_fps; }

    struct Sentinel {};
    class Iter {
        friend class DecodingStream;

     public:
        using value_type = Frame;
        using difference_type = std::ptrdiff_t;

        Iter() = default;
        explicit Iter(Sentinel /*unused*/) {}

        bool operator==(Sentinel /*unused*/) const noexcept { return !m_curr; }
        bool operator!=(Sentinel s) const noexcept { return !(*this == s); }

        Iter& operator++() {
            m_curr = m_buf->pop();
            if (!m_curr->width()) m_curr.reset();
            return *this;
        }

        void operator++(int) { ++*this; }

        Frame& operator*() const noexcept { return *m_curr; }

        Frame* operator->() const noexcept { return &*m_curr; }

     private:
        explicit Iter(RingBuffer<Frame>* buf)
            : m_buf(buf), m_curr(m_buf->pop()) {}
        RingBuffer<Frame>* m_buf{};
        mutable std::optional<Frame> m_curr{};
    };

    static_assert(std::input_iterator<Iter>);

    DecodingStream() = default;

    auto begin() { return Iter{m_buf}; }
    constexpr Sentinel end() const noexcept { return {}; }

    /**
     * \brief Whether the stream produces a sill image
     *
     * This effectively means that the resulting media has a single frame.
     * */
    [[nodiscard]] bool still() const noexcept { return m_still; }

 private:
    DecodingStream(RingBuffer<Frame>* buf, Frac<int> fps, bool still)
        : m_buf(buf), m_fps(fps), m_still(still) {}

    RingBuffer<Frame>* m_buf{};
    Frac<int> m_fps{0, 0};
    bool m_still{};
};
}  // namespace plai::media
