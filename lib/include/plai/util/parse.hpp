#pragma once

#include <concepts>
#include <optional>
#include <string_view>

namespace plai {

template <std::integral T>
constexpr std::optional<T> to_number(std::string_view s) {
    T out{};
    for (char c : s) {
        if (c < '0' || c > '9') return std::nullopt;
        out = (out * 10) + c - '0';
    }
    return out;
}

}  // namespace plai
