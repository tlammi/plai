#pragma once

#include <memory>
#include <plai/flow/connector.hpp>
#include <plai/flow/pipeline.hpp>
#include <plai/flow/sink.hpp>
#include <plai/flow/src.hpp>

namespace plai::flow {
namespace detail {
struct PipelineFinish {};

template <class T, class... Connectors>
class SpecBuilder {
 public:
    explicit constexpr SpecBuilder(Src<T>& src) noexcept : m_src(&src) {}
    explicit constexpr SpecBuilder(
        Src<T>& src, std::tuple<Connectors...> connectors) noexcept
        : m_src(&src), m_connectors(std::move(connectors)) {}

    template <std::derived_from<Sink<T>> Snk>
    auto operator|(Snk& sink) && {
        using NewConnector = Connector<T, T>;
        auto new_connectors =
            std::tuple_cat(std::move(m_connectors),
                           std::make_tuple(NewConnector(*m_src, sink)));
        if constexpr (src_type<std::remove_cvref_t<Snk>>) {
            return SpecBuilder<typename Snk::produced_type, Connectors...,
                               NewConnector>(sink, std::move(new_connectors));
        } else {
            return SpecBuilder<void, Connectors..., NewConnector>(
                std::move(new_connectors));
        }
    }

 private:
    Src<T>* m_src;
    std::tuple<Connectors...> m_connectors{};
};

template <class... Connectors>
class SpecBuilder<void, Connectors...> {
 public:
    SpecBuilder(std::tuple<Connectors...>&& connectors)
        : m_connectors(std::move(connectors)) {}

    std::unique_ptr<Pipeline> operator|(PipelineFinish) && {
        return std::make_unique<detail::PipelineImpl<Connectors...>>(
            std::move(m_connectors));
    }

 private:
    std::tuple<Connectors...> m_connectors{};
};
struct PipelineStart {
    constexpr std::unique_ptr<Pipeline> operator|(
        PipelineFinish) const&& noexcept {
        return nullptr;
    }

    template <class T>
    constexpr auto operator|(Src<T>& src) && {
        return SpecBuilder(src);
    }
};
}  // namespace detail

constexpr detail::PipelineStart pipeline_start() noexcept { return {}; }

constexpr detail::PipelineFinish pipeline_finish() noexcept { return {}; }

}  // namespace plai::flow
