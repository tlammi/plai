#pragma once

#include <mutex>
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

    void step() const {
        sched::post(m_exec, [&] {
            if (m_src->data_available() && m_sink->ready())
                do_consume(m_src->produce());
        });
    }

 private:
    void do_consume(I val) {
        if (m_sink_exec)
            sched::post(m_sink_exec, [&, v = std::move(val)]() mutable {
                m_sink->consume(std::move(v));
            });
        else
            m_sink->consume(std::move(val));
    }

    void src_data_available() override {
        sched::post(m_exec, [&] {
            if (!m_sink->ready()) return;
            do_consume(m_src->produce());
        });
    }
    void sink_ready() override {
        sched::post(m_exec, [&] {
            if (!m_src->data_available()) return;
            do_consume(m_src->produce());
        });
    }

    sched::Executor m_exec{};
    sched::Executor m_sink_exec{};
    Src<I>* m_src;
    Sink<O>* m_sink;
};

}  // namespace plai::flow
