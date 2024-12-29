#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace plai {

constexpr std::pair<std::string_view, std::string_view> split_left(
    std::string_view in, std::string_view delim) {
    auto idx = in.find(delim);
    if (idx == std::string_view::npos) return {in, {}};
    return {in.substr(0, idx), in.substr(idx + delim.size())};
}

constexpr std::pair<std::string_view, std::string_view> split_right(
    std::string_view in, std::string_view delim) {
    auto idx = in.rfind(delim);
    if (idx == std::string_view::npos) return {{}, in};
    return {in.substr(0, idx), in.substr(idx + delim.size())};
}

constexpr std::string to_lower(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(static_cast<char>(c - 'A') + 'a');
    }
    return s;
}

constexpr auto to_hex_str(std::span<const uint8_t> span) {
    std::string out(span.size() * 2, '\0');
    size_t idx = 0;
    static constexpr auto to_char = [](std::byte in) {
        auto c = std::to_integer<uint8_t>(in);
        // NOLINTBEGIN
        if (c >= 0x00 && c <= 0x09) return static_cast<char>(c + '0');
        return static_cast<char>(c - 0x0a + 'a');
        // NOLINTEND
    };
    for (const auto c : span) {
        auto msb = std::byte(c) >> 4;
        auto lsb = std::byte(c) & std::byte(0x0f);  // NOLINT
        auto msc = to_char(msb);
        auto lsc = to_char(lsb);
        out.at(idx) = msc;
        out.at(idx + 1) = lsc;
        idx += 2;
    }
    return out;
}

}  // namespace plai
