#pragma once

namespace plai::flow {

class SinkSubscriber {
 public:
    virtual ~SinkSubscriber() = default;

    virtual void sink_ready() = 0;
};

template <class T>
class Sink {
 public:
    virtual ~Sink() = default;

    virtual void consume(T val) = 0;
    virtual bool ready() = 0;

    virtual void on_sink_ready(SinkSubscriber* sub) = 0;
};
}  // namespace plai::flow
