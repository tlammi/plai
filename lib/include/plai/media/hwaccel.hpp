#pragma once

#include <cstddef>
#include <plai/media/forward.hpp>

namespace plai::media {

using hwaccel_t = int;

class HwAccelMeta {
 public:
    constexpr HwAccelMeta() noexcept = default;
    constexpr explicit HwAccelMeta(hwaccel_t type) noexcept : m_type(type) {}

    constexpr hwaccel_t type() const noexcept { return m_type; }
    const char* name() const noexcept;

 private:
    hwaccel_t m_type{};
};

/**
 * \brief Hardware acceleration
 * */
class HwAccel {
 public:
    constexpr HwAccel() = default;
    explicit HwAccel(const HwAccelMeta& meta) : HwAccel(meta.type()) {}
    explicit HwAccel(hwaccel_t type);

    HwAccel(const HwAccel& other);
    HwAccel& operator=(const HwAccel& other);

    HwAccel(HwAccel&& other) noexcept;
    HwAccel& operator=(HwAccel&& other) noexcept;

    ~HwAccel();

    [[nodiscard]] AVBufferRef* raw() const noexcept { return m_ref; }

    [[nodiscard]] hwaccel_t type() const noexcept;

    explicit operator bool() const noexcept;

 private:
    AVBufferRef* m_ref{};
};

class HwAccelRange {
 public:
    class Iter {
     public:
        using difference_type = std::ptrdiff_t;
        using value_type = HwAccelMeta;
        using reference = HwAccelMeta;
        using pointer_type = value_type*;
        constexpr Iter() noexcept {}

        constexpr explicit Iter(hwaccel_t type) noexcept : m_meta(type) {}

        Iter& operator++() noexcept;

        Iter operator++(int) noexcept {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        reference operator*() noexcept { return m_meta; }
        pointer_type operator->() noexcept { return &m_meta; }

        constexpr bool operator==(const Iter& other) const noexcept {
            return m_meta.type() == other.m_meta.type();
        }

     private:
        // really an enumeration in ffmpeg, but avoid polluting global namespace
        HwAccelMeta m_meta{};
    };

    constexpr explicit HwAccelRange(hwaccel_t first) noexcept
        : m_first(first) {}

    Iter begin() const noexcept;
    Iter end() const noexcept;

 private:
    hwaccel_t m_first{};
};

HwAccelRange supported_hardware_accelerators() noexcept;

/**
 * \brief Reference hardware accelerator by name
 *
 * Returns a matching hardware accelerator. The returning HWAccel is
 * contextually convertible to false if no matching accelerator was found.
 * */
HwAccel lookup_hardware_accelerator(const char* name);

}  // namespace plai::media
