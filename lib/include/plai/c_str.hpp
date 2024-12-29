#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace plai {

/**
 * \brief String view with quaranteed null termination
 * */
class CStr {
 public:
    constexpr CStr(const char* c, size_t len) noexcept : m_span(c, len) {}

    constexpr CStr(const char* c) noexcept
        : CStr(c, std::string_view(c).size()) {}

    constexpr CStr(const std::string& str) noexcept
        : m_span(str.c_str(), str.size()) {}

    constexpr const char* c_str() const noexcept { return m_span.data(); }
    constexpr operator const char*() const noexcept { return m_span.data(); }

    /// Number of bytes in the string WITHOUT the null terminator
    constexpr size_t size() const noexcept { return m_span.size(); }

    constexpr void remove_prefix(size_t count) {
        m_span = m_span.subspan(count);
    }

    constexpr std::string_view substr(size_t offset,
                                      size_t count) const noexcept {
        return std::string_view(m_span).substr(offset, count);
    }

    constexpr CStr substr(size_t offset) const noexcept {
        auto cut = m_span.subspan(offset);
        return CStr(cut.data(), cut.size());
    }

    constexpr std::string_view view() const noexcept {
        return std::string_view(m_span);
    }

 private:
    std::span<const char> m_span;
};
}  // namespace plai
