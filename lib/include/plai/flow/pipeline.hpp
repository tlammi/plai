#pragma once

#include <cstddef>
#include <tuple>

namespace plai::flow {

namespace pipeline_detail {

template <size_t Idx, class Fn, class... Ts>
void tuple_foreach_impl(Fn fn, std::tuple<Ts...>& tupl) {
    if constexpr (Idx < sizeof...(Ts)) {
        fn(std::get<Idx>(tupl));
        tuple_foreach_impl<Idx + 1>(fn, tupl);
    }
}

template <class Fn, class... Ts>
void tuple_foreach(Fn fn, std::tuple<Ts...>& tupl) {
    tuple_foreach_impl<0>(fn, tupl);
}
}  // namespace pipeline_detail

class Pipeline {
 public:
    virtual ~Pipeline() = default;
};

namespace detail {

template <class... Connectors>
class PipelineImpl final : public Pipeline {
 public:
    explicit PipelineImpl(std::tuple<Connectors...> connectors) noexcept
        : m_connectors(std::move(connectors)) {}

    void bootstrap() {
        pipeline_detail::tuple_foreach(
            [](auto& connector) { connector.bootstrap(); }, m_connectors);
    }

 private:
    std::tuple<Connectors...> m_connectors{};
};

}  // namespace detail
}  // namespace plai::flow
