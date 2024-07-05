#pragma once

#include <array>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <plai/util/defer.hpp>

namespace plai {
template <class T, size_t S>
class RingBuffer {
 public:
  RingBuffer() noexcept = default;

  RingBuffer(const RingBuffer&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

  RingBuffer(RingBuffer&&) noexcept = default;
  RingBuffer& operator=(RingBuffer&&) noexcept = default;

  ~RingBuffer() {
    if (!m_buf) return;
    while (m_count) {
      (*m_buf)[m_first].val.~T();
      m_first = (m_first + 1) % capacity();
      --m_count;
    }
  }

  constexpr size_t size() const noexcept {
    std::unique_lock lk{m_mut};
    return m_count;
  }
  constexpr size_t capacity() const noexcept { return S; }

  template <class... Ts>
  void emplace(Ts&&... ts) {
    std::unique_lock lk{m_mut};
    m_produce_cv.wait(lk, [&] { return m_count < capacity(); });
    auto idx = (m_first + m_count) % capacity();
    std::construct_at(&(*m_buf)[idx], std::forward<Ts>(ts)...);
    ++m_count;
  }

  template <class... Ts>
  bool try_emplace(Ts&&... ts) {
    std::unique_lock lk{m_mut};
    if (m_count >= capacity()) return false;
    auto idx = (m_first + m_count) % capacity();
    std::construct_at(&(*m_buf)[idx], std::forward<Ts>(ts)...);
    ++m_count;
    return true;
  }

  T pop() {
    std::unique_lock lk{m_mut};
    m_consume_cv.wait(lk, [&] { return m_count; });
    auto idx = m_first;
    m_first = (m_first + 1) % capacity();
    --m_count;
    Defer d{[&] { (*m_buf)[idx].val.~T(); }};
    return std::move((*m_buf)[idx].val);
  }

  std::optional<T> try_pop() {
    std::unique_lock lk{m_mut};
    if (!m_count) return std::nullopt;
    auto idx = m_first;
    m_first = (m_first + 1) % capacity();
    --m_count;
    Defer d{[&] { (*m_buf)[idx].val.~T(); }};
    return std::move((*m_buf)[idx].val);
  }

 private:
  union Elem {
    char dummy;
    T val;
  };
  using Arr = std::array<Elem, S>;
  using ArrPtr = std::unique_ptr<Arr>;
  ArrPtr m_buf{std::make_unique<Arr>()};
  size_t m_first{};
  size_t m_count{};
  mutable std::mutex m_mut{};
  std::condition_variable m_produce_cv{};
  std::condition_variable m_consume_cv{};
};
}  // namespace plai
