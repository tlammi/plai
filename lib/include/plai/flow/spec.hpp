#pragma once

#include <plai/flow/connector.hpp>
#include <plai/flow/sink.hpp>
#include <plai/flow/src.hpp>

namespace plai::flow {
class Spec {};
struct PipelineFinish {};

template <class T, class... Connectors>
class SpecBuilder {
 public:
    explicit constexpr SpecBuilder(Src<T>& src) noexcept : m_src(&src) {}

    auto operator|(Sink<T>& sink) {
        using NewConnector = Connector<T, T>;
        auto new_connectors =
            std::tuple_cat(std::move(m_connectors),
                           std::make_tuple(NewConnector(*m_src, sink)));
        return SpecBuilder<void, Connectors..., NewConnector>(
            std::move(new_connectors));
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

    auto operator|(PipelineFinish) { return Spec{}; }

 private:
    std::tuple<Connectors...> m_connectors{};
};
struct PipelineStart {
    constexpr auto operator|(PipelineFinish) const noexcept { return Spec{}; }

    template <class T>
    constexpr auto operator|(Src<T>& src) {
        return SpecBuilder(src);
    }
};

constexpr PipelineStart pipeline_start() noexcept { return {}; }

constexpr PipelineFinish pipeline_finish() noexcept { return {}; }

}  // namespace plai::flow
