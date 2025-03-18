#pragma once

#include <compare>
#include <format>
#include <map>
#include <plai/exceptions.hpp>
#include <plai/util/view_generator.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace plai::net::http {

class Target {
 public:
    using Params = std::map<std::string_view, std::string_view, std::less<>>;

    Target() = default;
    Target(std::string_view target, Params params)
        : m_tgt(target), m_pars(std::move(params)) {}

    std::string_view at(std::string_view param) const {
        return m_pars.at(param);
    }

    const Params& path_params() const noexcept { return m_pars; }

    std::strong_ordering operator<=>(std::string_view other) const noexcept {
        return std::strong_ordering::equal;
    }

    bool operator==(std::string_view other) const noexcept {
        return (*this <=> other) == std::strong_ordering::equal;
    }

 private:
    std::string_view m_tgt{};
    Params m_pars{};
};

std::optional<Target> parse_target(std::string_view pattern,
                                   std::string_view tgt);

}  // namespace plai::net::http
