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

    /**
     * \brief Specify executor to use to invoke the interface
     *
     * If this returns an object that is contextually converted to true the
     * object is used to call the methods in this interface. If the object has
     * value of false the methods may be called from any executor or outside of
     * any executor.
     *
     * Switching the returned object is undefined behavior. This may be called
     * once or multiple times by the consumer.
     * */
    virtual sched::Executor executor() { return {}; }
};
}  // namespace plai::flow
