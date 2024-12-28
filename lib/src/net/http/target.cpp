#include <plai/net/http/target.hpp>
namespace plai::net::http {
namespace rv = std::ranges::views;

std::optional<Target> parse_target(std::string_view pattern,
                                   std::string_view tgt) {
    Target::Params params{};
    auto pattern_view = rv::split(pattern, '/') | rv::transform([](auto in) {
                            return std::string_view(in);
                        });
    auto tgt_view = rv::split(tgt, '/') |
                    rv::transform([](auto in) { return std::string_view(in); });
    auto pattern_iter = pattern_view.begin();
    auto tgt_iter = tgt_view.begin();
    while (pattern_iter != pattern_view.end() && tgt_iter != tgt_view.end()) {
        auto p = *pattern_iter;
        auto t = *tgt_iter;
        if (p.starts_with('{')) {
            p.remove_prefix(1);
            if (!p.ends_with('}')) throw ValueError("invalid pattern");
            p.remove_suffix(1);
            params[p] = t;
        } else if (p != t) {
            return std::nullopt;
        }
        ++pattern_iter;
        ++tgt_iter;
    }
    if (pattern_iter != pattern_view.end() || tgt_iter != tgt_view.end())
        return std::nullopt;
    return Target(tgt, std::move(params));
}

}  // namespace plai::net::http
