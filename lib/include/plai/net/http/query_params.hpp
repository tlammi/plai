#pragma once

#include <map>
#include <plai/util/str.hpp>
#include <ranges>
#include <string_view>

namespace plai::net::http {

using QueryParams = std::map<std::string_view, std::string_view, std::less<>>;

inline QueryParams parse_query_params(std::string_view param_str) {
    namespace rv = std::ranges::views;
    QueryParams out{};
    for (auto pair : rv::split(param_str, "&")) {
        auto [k, v] = split_left(std::string_view(pair), "=");
        out[k] = v;
    }
    return out;
}

}  // namespace plai::net::http
