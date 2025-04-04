#pragma once

#include <cstdint>
#include <plai/net/http/method.hpp>
#include <plai/net/http/query_params.hpp>
#include <plai/net/http/target.hpp>

namespace plai::net::http {

class Request {
 public:
    virtual Method method() const noexcept = 0;
    virtual const Target& target() const = 0;

    /**
     * \brief Request parameters
     * */
    virtual const QueryParams& query_params() const = 0;

    // TODO: make not const
    virtual std::string_view text() const = 0;
    virtual std::optional<std::string_view> text_chunked() const = 0;

    std::span<const uint8_t> data() const {
        auto v = text();
        return {reinterpret_cast<const uint8_t*>(v.data()) /*NOLINT*/,
                v.size()};
    }

    std::optional<std::span<const uint8_t>> data_chunked() const {
        return text_chunked().transform([](auto&& v) {
            return std::span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(v.data()), v.size());
        });
    }

 protected:
    constexpr ~Request() = default;
};
}  // namespace plai::net::http
