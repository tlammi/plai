/**
 * \brief file
 * */
#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <plai/util/tags.hpp>
#include <utility>
#include <vector>

namespace plai {

/**
 * \brief Ring buffer with persistent elements
 *
 * Unlike a normal ring buffer the buffer is always full allowing
 * the consumer to pass the objects eventually back to the producer.
 * */
template <class T>
class PersistBuffer {
 public:
    explicit PersistBuffer(size_t count) : m_buf(count) {}

    template <class F>
    PersistBuffer(size_t count, factory_t /*unused*/, F f) {
        m_buf.reserve(count);
        while (count--) m_buf.emplace_back(f());
    }

    size_t size() const noexcept {
        auto lk = std::unique_lock(m_mut);
        return m_buf.size();
    }

    size_t length() const noexcept { return size(); }

    bool empty() const noexcept { return size() == 0; }

    size_t capacity() const noexcept {
        auto lk = std::unique_lock(m_mut);
        return m_buf.size();
    }

    T push(T replace) {
        auto lk = std::unique_lock(m_mut);
        m_space.wait(lk, [&] { return m_count < m_buf.size(); });
        auto res = std::exchange(m_buf.at((m_offset + m_count) % m_buf.size()),
                                 std::move(replace));
        ++m_count;
        lk.unlock();
        m_data.notify_one();
        return res;
    }

    T pop(T replace) {
        auto lk = std::unique_lock(m_mut);
        m_data.wait(lk, [&] { return m_count > 0; });
        auto res = std::exchange(m_buf.at(m_offset), std::move(replace));
        m_offset = (m_offset + 1) % m_buf.size();
        --m_count;
        lk.unlock();
        m_space.notify_one();
        return res;
    }

 private:
    mutable std::mutex m_mut{};
    std::condition_variable m_space{};
    std::condition_variable m_data{};
    std::vector<T> m_buf{};
    size_t m_offset{};
    size_t m_count{};
};
}  // namespace plai
