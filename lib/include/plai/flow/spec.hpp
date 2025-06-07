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
    explicit constexpr SpecBuilder(sched::Executor exec, Src<T>& src) noexcept
        : m_exec(std::move(exec)), m_src(&src) {}
    explicit constexpr SpecBuilder(
        sched::Executor exec, Src<T>& src,
        std::tuple<Connectors...> connectors) noexcept
        : m_exec(std::move(exec)),
          m_src(&src),
          m_connectors(std::move(connectors)) {}

    template <std::derived_from<Sink<T>> Snk>
    auto operator|(Snk& sink) && {
        using NewConnector = Connector<T, T>;
        assert(m_exec);
        auto new_connectors =
            std::tuple_cat(std::move(m_connectors),
                           std::make_tuple(NewConnector(m_exec, *m_src, sink)));
        if constexpr (src_type<std::remove_cvref_t<Snk>>) {
            return SpecBuilder<typename Snk::produced_type, Connectors...,
                               NewConnector>(m_exec, sink,
                                             std::move(new_connectors));
        } else {
            return SpecBuilder<void, Connectors..., NewConnector>(
                std::move(new_connectors));
        }
    }

 private:
    sched::Executor m_exec;
    Src<T>* m_src;
    std::tuple<Connectors...> m_connectors{};
};

template <class... Connectors>
class SpecBuilder<void, Connectors...> {
 public:
    SpecBuilder(std::tuple<Connectors...>&& connectors)
        : m_connectors(std::move(connectors)) {}

    std::unique_ptr<Pipeline> operator|(PipelineFinish) && {
        auto ptr = std::make_unique<detail::PipelineImpl<Connectors...>>(
            std::move(m_connectors));
        ptr->bootstrap();
        return ptr;
    }

 private:
    std::tuple<Connectors...> m_connectors{};
};
struct PipelineStart {
    sched::Executor exec;
    constexpr std::unique_ptr<Pipeline> operator|(
        PipelineFinish) const&& noexcept {
        return nullptr;
    }

    template <class T>
    constexpr auto operator|(Src<T>& src) && {
        assert(exec);
        return SpecBuilder(std::move(exec), src);
    }
};
}  // namespace detail

template <class T>
concept executor_provider = requires(T t) {
    { t.get_executor() } -> std::convertible_to<sched::Executor>;
};

constexpr detail::PipelineStart pipeline(sched::Executor exec) {
    return {.exec = std::move(exec)};
}

template <executor_provider T>
constexpr detail::PipelineStart pipeline(T& t) {
    return pipeline(t.get_executor());
}

constexpr detail::PipelineFinish pipeline_finish() noexcept { return {}; }

}  // namespace plai::flow
