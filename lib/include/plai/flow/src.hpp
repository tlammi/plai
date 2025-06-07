#pragma once

namespace plai::flow {

class SrcSubscriber {
 public:
    virtual ~SrcSubscriber() = default;
    virtual void src_data_available() = 0;
};

template <class T>
class Src {
 public:
    virtual ~Src() = default;

    virtual T produce() = 0;
    virtual bool data_available() = 0;

    virtual void on_data_available(SrcSubscriber* sub) = 0;
};
}  // namespace plai::flow
