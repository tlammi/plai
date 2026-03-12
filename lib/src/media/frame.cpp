
#include <cassert>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <plai/media/frame.hpp>
#include <utility>

namespace plai::media {

Frame::Frame() : m_raw(av_frame_alloc()) {
    if (!m_raw) throw std::bad_alloc();
}

Frame::Frame(const Frame& other) : Frame() {
    int res = av_frame_ref(m_raw, other.m_raw);
    // TODO: Gracefully check and avoid memory leaks
    assert(!res);
}

Frame& Frame::operator=(const Frame& other) {
    av_frame_unref(m_raw);
    av_frame_ref(m_raw, other.m_raw);
    return *this;
}

Frame::Frame(Frame&& other) noexcept
    : m_raw(std::exchange(other.m_raw, nullptr)) {}

Frame& Frame::operator=(Frame&& other) noexcept {
    auto tmp = Frame(std::move(other));
    std::swap(m_raw, tmp.m_raw);
    return *this;
}

Frame::~Frame() {
    if (m_raw) {
        av_frame_unref(m_raw);
        av_frame_free(&m_raw);
    }
}

Frame::operator bool() const noexcept {
    if (!m_raw) return false;
    if (!m_raw->width) return false;
    if (!m_raw->height) return false;
    return true;
}

int Frame::width() const noexcept { return m_raw->width; }
int Frame::height() const noexcept { return m_raw->height; }

}  // namespace plai::media
