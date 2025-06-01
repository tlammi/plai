#pragma once

#include <condition_variable>
#include <mutex>
#include <plai/util/defer.hpp>
#include <utility>

namespace plai {
namespace buf_detail {

template <class T>
union Union {
    constexpr Union() : unused{} {}

    Union(const Union&) = delete;
    Union& operator=(const Union&) = delete;

    Union(Union&&) = delete;
    Union& operator=(Union&&) = delete;

    ~Union() {}
    uint8_t unused{};
    T value;
};

template <class T>
struct Ctx {
    Ctx() noexcept = default;
    explicit Ctx(size_t size) : capacity(size), buf(new Union<T>[size]) {}
    Ctx(const Ctx&) = delete;
    Ctx& operator=(const Ctx&) = delete;

    Ctx(Ctx&& other) noexcept
        : offset(other.offset),
          count(other.count),
          capacity(other.capacity),
          buf(std::exchange(other.buf, nullptr)) {}

    Ctx& operator=(Ctx&& other) noexcept {
        auto tmp = Ctx(std::move(other));
        std::swap(offset, tmp.offset);
        std::swap(count, tmp.count);
        std::swap(capacity, tmp.capacity);
        std::swap(buf, tmp.buf);
        return *this;
    }

    ~Ctx() {
        if (!buf) return;
        for (size_t i = 0; i < count; ++i) {
            // NOLINTNEXTLINE
            buf[(i + offset) % capacity].value.~T();
        }
        delete[] buf;
    }
    size_t offset{};
    size_t count{};
    size_t capacity{};
    Union<T>* buf{};
};
}  // namespace buf_detail

template <class T>
class RingBuffer {
 public:
    explicit RingBuffer(size_t size) : m_ctx(size) {}

    size_t size() const noexcept {
        auto lk = std::unique_lock(m_mut);
        return m_ctx.count;
    }
    size_t length() const noexcept { return size(); }
    bool empty() const noexcept { return size() == 0; }

    size_t capacity() const noexcept {
        auto lk = std::unique_lock(m_mut);
        return m_ctx.capacity;
    }

    void push(const T& t) { emplace(t); }
    void push(T&& t) { emplace(std::move(t)); }

    template <class... Ts>
    void emplace(Ts&&... ts) {
        auto defer = Defer([&] { m_data.notify_one(); });
        auto lk = std::unique_lock(m_mut);
        m_space.wait(lk, [&] { return m_ctx.count < m_ctx.capacity; });
        auto idx = (m_ctx.offset + m_ctx.count) % m_ctx.capacity;
        std::construct_at(&m_ctx.buf[idx].value, std::forward<Ts>(ts)...);
        ++m_ctx.count;
    }

    template <class... Ts>
    bool try_emplace(Ts&&... ts) {
        auto defer = Defer([&] { m_data.notify_one(); });
        auto lk = std::unique_lock(m_mut);
        if (m_ctx.count == m_ctx.capacity) {
            defer.cancel();
            return false;
        }
        auto idx = (m_ctx.offset + m_ctx.count) & m_ctx.capacity;
        std::construct_at(&m_ctx.buf[idx].value, std::forward<Ts>(ts)...);
        ++m_ctx.count;
        return true;
    }

    T pop() {
        auto defer = Defer([&] { m_space.notify_one(); });
        auto lk = std::unique_lock(m_mut);
        m_data.wait(lk, [&] { return m_ctx.count > 0; });
        auto offset = m_ctx.offset++;
        --m_ctx.count;
        m_ctx.offset %= m_ctx.capacity;
        auto res = std::move(m_ctx.buf[offset].value);
        m_ctx.buf[offset].value.~T();
        return res;
    }

    std::optional<T> try_pop() {
        auto defer = Defer([&] { m_space.notify_one(); });
        auto lk = std::unique_lock(m_mut);
        if (m_ctx.count == 0) {
            defer.cancel();
            return std::nullopt;
        }
        auto offset = m_ctx.offset++;
        --m_ctx.count;
        m_ctx.offset %= m_ctx.capacity;
        auto res = std::move(m_ctx.buf[offset].value);
        m_ctx.buf[offset].value.~T();
        return res;
    }

 private:
    mutable std::mutex m_mut{};
    std::condition_variable m_space{};
    std::condition_variable m_data{};
    buf_detail::Ctx<T> m_ctx{};
};
}  // namespace plai
