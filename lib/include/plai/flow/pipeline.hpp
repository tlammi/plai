#pragma once

#include <tuple>

namespace plai::flow {

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

 private:
    std::tuple<Connectors...> m_connectors{};
};

}  // namespace detail
}  // namespace plai::flow
