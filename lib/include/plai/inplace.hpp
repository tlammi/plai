#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace plai {

template <class T, class U>
concept inplaceable = std::derived_from<T, U> || std::same_as<T, U>;
template <class T, size_t S>
class Inplace {
 public:
    constexpr Inplace() noexcept = default;

    template <inplaceable<T> U, class... Ts>
    explicit Inplace(std::in_place_type_t<U> /*unused*/, Ts&&... ts) {
        static_assert(sizeof(U) <= S);
        static_assert(std::movable<U>);
        std::construct_at(buffer<U>(), std::forward<Ts>(ts)...);
        // NOLINTNEXTLINE
        m_move = +[](void* s, void* d) noexcept {
            auto* src = static_cast<U*>(s);
            auto* dst = static_cast<U*>(d);
            std::construct_at(dst, std::move(*src));
        };
    }

    Inplace(const Inplace&) = delete;
    Inplace& operator=(const Inplace&) = delete;

    Inplace(Inplace&& other) noexcept
        : m_move(std::exchange(other.m_move, nullptr)) {
        if (m_move) {
            m_move(other.get(), get());
            other.get()->~T();
        }
    }

    Inplace& operator=(Inplace&& other) noexcept {
        auto tmp = std::move(*this);
        m_move = std::exchange(other.m_move, nullptr);
        if (m_move) {
            m_move(other.get(), get());
            other.get()->~T();
        }
        return *this;
    }

    ~Inplace() {
        if (m_move) get()->~T();
    }

    T* get() noexcept { return buffer<T>(); }
    const T* get() const noexcept { return buffer<const T>(); }

    T* operator->() noexcept { return get(); }
    const T* operator->() const noexcept { return get(); }

    T& operator*() noexcept { return *get(); }
    const T& operator*() const noexcept { return *get(); }

    explicit operator bool() const noexcept { return m_move; }

 private:
    template <class U>
    U* buffer() noexcept {
        return reinterpret_cast<U*>(m_buf.data());  // NOLINT
    }
    std::array<std::uint8_t, S> m_buf{};
    void (*m_move)(void*, void*) noexcept {};
};

template <class Base, class T, size_t S = sizeof(T), class... Ts>
auto make_inplace(Ts&&... ts) {
    return Inplace<Base, S>(std::in_place_type<T>, std::forward<Ts>(ts)...);
}

}  // namespace plai
