#pragma once

#include <mutex>
#include <plai/flow/sink.hpp>
#include <plai/flow/src.hpp>
#include <utility>

namespace plai::flow {

template <class I, class O>
class Connector final : SrcSubscriber, SinkSubscriber {
 public:
    Connector(Src<I>& src, Sink<O>& sink) noexcept
        : m_src(&src), m_sink(&sink) {}

    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;

    Connector(Connector&& other) noexcept
        : m_src(std::exchange(other.m_src, nullptr)),
          m_sink(std::exchange(other.m_sink, nullptr)) {
        if (m_src) {
            m_src->on_data_available(this);
            m_sink->on_sink_ready(this);
        }
    }

    Connector& operator=(Connector&&) = delete;

    ~Connector() override {
        if (m_src) {
            m_src->on_data_available(nullptr);
            m_sink->on_sink_ready(nullptr);
        }
    }

 private:
    void src_data_available() override {
        auto lk = std::unique_lock(m_mut);
        if (!m_sink->ready()) return;
        m_sink->consume(m_src->produce());
    }
    void sink_ready() override {
        auto lk = std::unique_lock(m_mut);
        if (!m_src->data_available()) return;
        m_sink->consume(m_src->produce());
    }

    // Maybe it is enough to have a normal mutex?
    // The problems happen e.g. with src_data_available -> consume -> sink_ready
    std::recursive_mutex m_mut{};
    Src<I>* m_src;
    Sink<O>* m_sink;
};

}  // namespace plai::flow
