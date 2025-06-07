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
    flow::SinkSubscriber* subscriber{};
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

template <class T, class U>
class Proxy final : public flow::Sink<T>, public flow::Src<U> {
 public:
    explicit Proxy(std::function<U(T)> converter) : m_conv(converter) {}

    U produce() override { return *std::exchange(m_buf, std::nullopt); }
    bool data_available() override { return m_buf.has_value(); }

    void on_data_available(flow::SrcSubscriber* sub) override {}

    void consume(T val) override { m_buf.emplace(m_conv(std::move(val))); }
    bool ready() override { return !m_buf.has_value(); }

    void on_sink_ready(flow::SinkSubscriber* sub) override {}

 private:
    std::function<U(T)> m_conv;
    std::optional<U> m_buf{};
};

TEST(Construct, SrcProxySink) {
    auto src = DummySrc<int>();
    auto proxy = Proxy<int, float>([](int i) { return float(i); });
    auto sink = DummySink<float>();

    auto pipeline =
        flow::pipeline_start() | src | proxy | sink | flow::pipeline_finish();
}
