
extern "C" {
#include <libavcodec/avcodec.h>
}

#include <plai/media/frame.hpp>
#include <utility>

namespace plai::media {

Frame::Frame() : m_raw(av_frame_alloc()) {
    if (!m_raw) throw std::bad_alloc();
}
Frame::Frame(Frame&& other) noexcept
    : m_raw(std::exchange(other.m_raw, nullptr)),
      m_is_dyn(std::exchange(other.m_is_dyn, false)) {}

Frame& Frame::operator=(Frame&& other) noexcept {
    auto tmp = Frame(std::move(other));
    std::swap(m_raw, tmp.m_raw);
    std::swap(m_is_dyn, tmp.m_is_dyn);
    return *this;
}

Frame::~Frame() {
    if (m_raw) {
        if (m_is_dyn) av_freep(&m_raw->data);
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
