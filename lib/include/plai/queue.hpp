#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>

namespace plai {
namespace queue_detail {

constexpr size_t CHUNK_SIZE_HINT = 512;

template <class T>
struct chunk_elems {
    static constexpr size_t value = CHUNK_SIZE_HINT % sizeof(T) + 1;
};

template <class I, class T>
struct can_fit {
    static constexpr bool value =
        chunk_elems<T>::value < std::numeric_limits<I>::max();
};

template <class T, class = void>
struct chunk_index {};

template <class T>
struct chunk_index<T, std::enable_if_t<can_fit<uint8_t, T>::value>> {
    using type = uint8_t;
};

template <class T>
struct chunk_index<T, std::enable_if_t<!can_fit<uint8_t, T>::value &&
                                       can_fit<uint16_t, T>::value>> {
    using type = uint16_t;
};

template <class T>
using chunk_index_t = typename chunk_index<T>::type;

template <class T>
union ChunkElem {
    ChunkElem() noexcept = default;
    ChunkElem(const ChunkElem&) = delete;
    ChunkElem& operator=(const ChunkElem&) = delete;
    ChunkElem(ChunkElem&&) = delete;
    ChunkElem& operator=(ChunkElem&&) = delete;
    ~ChunkElem() {}
    uint8_t dummy{};
    T value;
};

template <class T>
struct Chunk {
    Chunk() {}
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&&) = delete;
    Chunk& operator=(Chunk&&) = delete;

    ~Chunk() {
        for (auto i = 0; i < count; ++i) { values[offset + i].value.~T(); }
    }

    std::unique_ptr<Chunk> next{};
    chunk_index_t<T> offset{};
    chunk_index_t<T> count{};
    std::array<ChunkElem<T>, chunk_elems<T>::value> values{};
};

}  // namespace queue_detail

template <class T>
class Queue {
 public:
    constexpr Queue() noexcept = default;

    template <class... Ts>
    Queue(std::in_place_t /*unused*/, Ts&&... ts) {
        (push_back(std::forward<Ts>(ts)), ...);
    }

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(Queue&& other) noexcept
        : m_front(std::exchange(other.m_front, nullptr)),
          m_back(std::exchange(other.m_back, nullptr)) {}

    Queue& operator=(Queue&& other) noexcept {
        auto tmp = Queue(std::move(other));
        std::swap(m_front, tmp.m_front);
        std::swap(m_back, tmp.m_back);
        return *this;
    }

    ~Queue() = default;

    size_t size() const noexcept {
        const auto* ptr = m_front.get();
        size_t res{};
        while (ptr) {
            res += ptr->count;
            ptr = ptr->next.get();
        }
        return res;
    }
    size_t length() const noexcept { return size(); }
    bool empty() const noexcept { return !m_front || m_front->count == 0; }

    template <class S>
    decltype(auto) front(this S&& self) noexcept {
        assert(!self.empty());
        return std::forward<S>(self).m_front->values[0].value;
    }

    template <class S>
    decltype(auto) back(this S&& self) noexcept {
        assert(!self.empty());
        auto offset = self.m_back->offset;
        auto count = self.m_back->count;
        return std::forward<S>(self).m_back->values[offset + count - 1].value;
    }

    void push_back(const T& t) { emplace_back(t); }
    void push_back(T&& t) { emplace_back(std::move(t)); }

    template <class... Ts>
    void emplace_back(Ts&&... ts) {
        prepare_push_back();
        auto offset = m_back->offset;
        auto& count = m_back->count;
        std::construct_at(&m_back->values[offset + count].value,
                          std::forward<Ts>(ts)...);
        ++count;
    }

    void push_front(const T& t) { emplace_front(t); }
    void push_front(T&& t) { emplace_front(std::move(t)); }

    template <class... Ts>
    void emplace_front(Ts&&... ts) {
        if (!m_front) {
            m_front = std::make_unique<queue_detail::Chunk<T>>();
            m_back = m_front.get();
            m_front->offset = m_front.values.size();
        } else if (!m_front->offset && !m_front->count) {
            // empty chunk at the start
            m_front->offset = m_front->values.size();
        } else if (!m_front->offset && m_front->count) {
            // no free indexes in the current chunk
            if (!m_back->next) {
                auto second = std::exchange(
                    m_front, std::make_unique<queue_detail::Chunk<T>>());
                m_front->next = std::move(second);
                m_front->offset = m_front.values.size();
            } else {
                auto new_front =
                    std::exchange(m_back->next, std::move(m_back->next->next));
                new_front->next = std::move(m_front);
                m_front = std::move(new_front);
            }
        }
        std::construct_at(m_front->values[m_front->offset - 1].value,
                          std::forward<Ts>(ts)...);
        --m_front->offset;
        ++m_front->count;
    }

    void pop_back() {
        auto offset = m_back->offset;
        auto& count = m_back->count;
        m_back->values[offset + count - 1].value.~T();
        --count;
        if (!count && m_front.get() != m_back) {
            auto* ptr = m_front.get();
            while (ptr->next != m_back) { ptr = ptr->next.get(); }
            m_back = ptr;
        }
    }

    void pop_front() {
        auto& offset = m_front->offset;
        auto& count = m_front->count;
        m_front->values[offset].value.~T();
        ++offset;
        --count;
        if (m_front.get() != m_back) {
            auto tail = std::exchange(m_back->next, nullptr);
            auto new_front = std::exchange(m_front->next, std::move(tail));
            m_back->next = std::exchange(m_front, std::move(new_front));
        }
    }

 private:
    void prepare_push_back() {
        if (!m_front) {
            m_front = std::make_unique<queue_detail::Chunk<T>>();
            m_back = m_front.get();
        }
        if (m_back->offset + m_back->count == m_back->values.size()) {
            if (!m_back->next)
                m_back->next = std::make_unique<queue_detail::Chunk<T>>();
            m_back = m_back->next.get();
        }
    }

    std::unique_ptr<queue_detail::Chunk<T>> m_front{};
    queue_detail::Chunk<T>* m_back{};
};
}  // namespace plai

