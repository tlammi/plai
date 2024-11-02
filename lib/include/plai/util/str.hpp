#pragma once

#include <string_view>

namespace plai {

std::pair<std::string_view, std::string_view> split_left(
    std::string_view in, std::string_view delim) {
    auto idx = in.find(delim);
    if (idx == std::string_view::npos) return {in, {}};
    return {in.substr(0, idx), in.substr(idx + delim.size())};
}

std::pair<std::string_view, std::string_view> split_right(
    std::string_view in, std::string_view delim) {
    auto idx = in.rfind(delim);
    if (idx == std::string_view::npos) return {{}, in};
    return {in.substr(0, idx), in.substr(idx + delim.size())};
}

}  // namespace plai
