#pragma once
#include <plai/buffer.hpp>
#include <plai/virtual.hpp>

namespace plai::sched::detail {

class AnyStep {
 public:
    virtual ~AnyStep() = default;
    virtual void operator()() = 0;
};

template <class Fn>
class Step : public AnyStep {
 public:
    Step(Buffer& buf, Fn fn) : m_buf(&buf), m_fn(std::move(fn)) {}

    void operator()() override { m_buf->mutate(m_fn); }

 private:
    Buffer* m_buf;
    Fn m_fn;
};
}  // namespace plai::sched::detail
