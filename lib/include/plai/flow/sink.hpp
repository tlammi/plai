#pragma once

#include <plai/sched/executor.hpp>

namespace plai::flow {

/**
 * \brief Callback for sinks
 *
 * Used by Sink<T> to notify the subscriber (typically Connector) that it is
 * ready to consume items.
 * */
class SinkSubscriber {
 public:
    virtual ~SinkSubscriber() = default;

    /**
     * \brief Notify about Sink<T> being ready to receive more data
     * */
    virtual void sink_ready() = 0;
};

template <class T>
class Sink {
 public:
    virtual ~Sink() = default;

    /**
     * \brief Consume a new item
     * */
    virtual void consume(T val) = 0;

    virtual bool sink_ready() = 0;

    void set_subscriber(SinkSubscriber* sub) noexcept { m_sub = sub; }

 protected:
    void notify_sink_ready() {
        auto ptr = m_sub.load();
        if (ptr) ptr->sink_ready();
    }

 private:
    std::atomic<SinkSubscriber*> m_sub{};
};
}  // namespace plai::flow
