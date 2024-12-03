#pragma once

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

inline std::string to_lower(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(static_cast<char>(c - 'A') + 'a');
    }
    return s;
}

}  // namespace plai
