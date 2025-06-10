#pragma once

#include <plai/flow/sink.hpp>
#include <plai/flow/src.hpp>
#include <utility>

namespace plai::flow {

template <class I, class O>
class Connector final : SrcSubscriber, SinkSubscriber {
 public:
    Connector(sched::Executor exec, Src<I>& src, Sink<O>& sink) noexcept
        : m_exec(std::move(exec)), m_src(&src), m_sink(&sink) {}

    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;

    Connector(Connector&& other) noexcept
        : m_exec(std::move(other.m_exec)),
          m_src(std::exchange(other.m_src, nullptr)),
          m_sink(std::exchange(other.m_sink, nullptr)) {}

    Connector& operator=(Connector&&) = delete;

    ~Connector() override {
        if (m_src) {
            m_src->set_subscriber(nullptr);
            m_sink->set_subscriber(nullptr);
        }
    }
    void bootstrap() {
        m_src->set_subscriber(this);
        m_sink->set_subscriber(this);
        sched::post(m_exec, [&] {
            if (m_src->src_ready() && m_sink->sink_ready())
                m_sink->consume(m_src->produce());
        });
    }

 private:
    void src_ready() override {
        sched::post(m_exec, [&] {
            if (!m_sink->sink_ready()) return;
            m_sink->consume(m_src->produce());
        });
    }

    void sink_ready() override {
        sched::post(m_exec, [&] {
            if (!m_src->src_ready()) return;
            m_sink->consume(m_src->produce());
        });
    }

    sched::Executor m_exec{};
    Src<I>* m_src;
    Sink<O>* m_sink;
};

}  // namespace plai::flow
