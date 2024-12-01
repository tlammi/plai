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

        bool operator==(Sentinel /*unused*/) const noexcept {
            return m_curr.width() == 0;
        }
        constexpr bool operator!=(Sentinel s) const noexcept {
            return !(*this == s);
        }

        Iter& operator++() {
            m_curr = m_buf->pop(std::move(m_curr));
            return *this;
        }

        void operator++(int) { ++*this; }

        const Frame& operator*() const noexcept { return m_curr; }

        const Frame* operator->() const noexcept { return &m_curr; }

     private:
        explicit Iter(PersistBuffer<Frame>* buf)
            : m_buf(buf), m_curr(m_buf->pop(Frame())) {}
        PersistBuffer<Frame>* m_buf;
        Frame m_curr{};
    };

    static_assert(std::input_iterator<Iter>);

    auto begin() { return Iter{m_buf}; }
    constexpr Sentinel end() const noexcept { return {}; }

 private:
    DecodingStream(PersistBuffer<Frame>* buf, Frac<int> fps)
        : m_buf(buf), m_fps(fps) {}

    PersistBuffer<Frame>* m_buf;
    Frac<int> m_fps;
};
}  // namespace plai::media
