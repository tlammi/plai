#include <gtest/gtest.h>

#include <plai/flow/spec.hpp>

namespace flow = plai::flow;
namespace sched = plai::sched;

template <class T>
struct Producer final : public flow::Src<T> {
    T produce() override { return *std::exchange(buf, std::nullopt); }
    bool data_available() override { return buf.has_value(); }

    void on_data_available(flow::SrcSubscriber* sub) override {
        this->sub = sub;
    }

    void push(T t) {
        buf.emplace(std::move(t));
        sub->src_data_available();
    }

    std::optional<T> buf{};
    flow::SrcSubscriber* sub{};
};

template <class T>
struct Consumer final : public flow::Sink<T> {
    void consume(T val) override {
        assert(!buf);
        buf.emplace(std::move(val));
    }

    bool ready() override { return !buf.has_value(); }

    void on_sink_ready(flow::SinkSubscriber* sub) override { this->sub = sub; }

    std::optional<T> buf{};
    flow::SinkSubscriber* sub{};
};

TEST(Pipeline, BootstrapNop) {
    auto p = Producer<int>();
    auto c = Consumer<int>();
    auto ctx = sched::IoContext();
    auto pline = flow::pipeline(ctx) | p | c | flow::pipeline_finish();
    ctx.run();
    ASSERT_FALSE(c.buf);
}
