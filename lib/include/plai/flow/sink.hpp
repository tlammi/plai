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

    /**
     * \brief Query whether the Sink<T> can receive more data
     * */
    virtual bool ready() = 0;

    /**
     * \brief Set subscriber
     *
     * The argument may be null to indicate that there is no subscriber.
     * */
    virtual void on_sink_ready(SinkSubscriber* sub) = 0;
};
}  // namespace plai::flow
