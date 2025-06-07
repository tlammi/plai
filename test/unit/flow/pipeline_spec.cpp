#include <gtest/gtest.h>

#include <plai/flow/spec.hpp>

namespace flow = plai::flow;

TEST(Construct, Null) {
    auto spec = flow::pipeline_start() | flow::pipeline_finish();
    (void)spec;
}

template <class T>
struct DummySrc final : public flow::Src<T> {
    std::optional<T> buffer{};
    flow::SrcSubscriber* subscriber{};
    T produce() override { return *std::exchange(buffer, std::nullopt); }
    bool data_available() override { return buffer.has_value(); }

    void on_data_available(flow::SrcSubscriber* sub) override {
        subscriber = sub;
    }

    void do_produce(T val) {
        buffer.emplace(std::move(val));
        subscriber->src_data_available();
    }
};

template <class T>
class DummySink final : public flow::Sink<T> {
 public:
    std::vector<T> produced{};
    flow::SinkSubscriber* subscriber;
    void consume(T val) override { produced.push_back(std::move(val)); }

    bool ready() override { return true; }

    void on_sink_ready(flow::SinkSubscriber* sub) override { subscriber = sub; }
};

TEST(Construct, SrcSink) {
    auto src = DummySrc<int>();
    auto sink = DummySink<int>();

    auto spec = flow::pipeline_start() | src | sink | flow::pipeline_finish();
    (void)spec;
}

TEST(Produce, Simple) {
    auto src = DummySrc<int>();
    auto sink = DummySink<int>();
    auto spec = flow::pipeline_start() | src | sink | flow::pipeline_finish();
}
