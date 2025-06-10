#include <gtest/gtest.h>

#include <plai/flow/spec.hpp>

namespace flow = plai::flow;
namespace sched = plai::sched;

template <class T>
struct Producer final : public flow::Src<T> {
    T produce() override { return *std::exchange(buf, std::nullopt); }
    bool src_ready() override { return buf.has_value(); }

    void push(T t) {
        buf.emplace(std::move(t));
        flow::Src<T>::notify_src_ready();
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

    bool sink_ready() override { return !buf.has_value(); }

    std::optional<T> buf{};
    flow::SinkSubscriber* sub{};
};

class Pipeline : public ::testing::Test {
 public:
    Producer<int> int_src{};
    Consumer<int> int_sink{};
    sched::IoContext ctx{};
};

TEST_F(Pipeline, BootstrapNop) {
    auto pline =
        flow::pipeline(ctx) | int_src | int_sink | flow::pipeline_finish();
    ctx.run();
    ASSERT_FALSE(int_sink.buf);
}

TEST_F(Pipeline, BoostrapOp) {
    int_src.buf = 1;
    auto pline =
        flow::pipeline(ctx) | int_src | int_sink | flow::pipeline_finish();
    ctx.run();
    ASSERT_EQ(int_sink.buf, 1);
}

TEST_F(Pipeline, Produce) {
    auto pline =
        flow::pipeline(ctx) | int_src | int_sink | flow::pipeline_finish();
    sched::post(ctx, [&] { int_src.push(2); });
    ctx.run();
    ASSERT_EQ(int_sink.buf, 2);
}
