#pragma once
#include <boost/type_index.hpp>
#include <memory>
#include <plai/util/algorithm.hpp>
#include <vector>

namespace plai {
namespace buf_detail {

template <class T, class... Ts>
void construct_at(void* p, Ts&&... ts) {
    // NOLINTNEXTLINE
    std::construct_at(reinterpret_cast<T*>(p), std::forward<Ts>(ts)...);
}

template <class T>
void destructor(void* p) {
    std::destroy_at(static_cast<T*>(p));
}

}  // namespace buf_detail
template <class... Ts>
constexpr size_t min_buffer_size() {
    return max_of(sizeof(Ts)...);
}
class Buffer {
 public:
    constexpr Buffer() noexcept = default;

    constexpr explicit Buffer(size_t size) : m_buf(size) {}

    template <class T, class... Ts>
    explicit Buffer(std::in_place_type_t<T> tag, Ts&&... ts)
        : Buffer(min_buffer_size<T>()) {
        emplace(tag, std::forward<Ts>(ts)...);
    }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept
        : m_buf(std::move(other.m_buf)),
          m_dtor(std::exchange(other.m_dtor, nullptr)) {}

    Buffer& operator=(Buffer&& other) noexcept {
        auto tmp = std::move(other);
        std::swap(m_buf, tmp.m_buf);
        std::swap(m_dtor, tmp.m_dtor);
        return *this;
    }

    ~Buffer() {
        if (m_dtor) m_dtor(m_buf.data());
    }

    template <class T, class... Ts>
    void emplace(std::in_place_type_t<T> tag, Ts&&... ts) {
        if (m_dtor) m_dtor(m_buf.data());
        if (m_buf.size() < sizeof(T)) m_buf.resize(sizeof(T));
        buf_detail::construct_at<T>(m_buf.data(), std::forward<Ts>(ts)...);
        m_dtor = &buf_detail::destructor<T>;
#ifndef NDEBUG
        m_tindex = boost::typeindex::type_id<T>();
#endif
    }

    template <class T>
    T* get() noexcept {
        assert(m_tindex == boost::typeindex::type_id<T>());
        // NOLINTNEXTLINE
        return reinterpret_cast<T*>(m_buf.data());
    }

    template <class T>
    const T* get() const noexcept {
        assert(m_tindex == boost::typeindex::type_id<T>());
        // NOLINTNEXTLINE
        return reinterpret_cast<T*>(m_buf.data());
    }

 private:
    std::vector<uint8_t> m_buf{};
    void (*m_dtor)(void* p){};
#ifndef NDEBUG
    boost::typeindex::type_index m_tindex{};
#endif
};

}  // namespace plai
