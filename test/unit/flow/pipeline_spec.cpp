#include <gtest/gtest.h>

#include <plai/flow/spec.hpp>

namespace flow = plai::flow;
namespace sched = plai::sched;

TEST(Construct, Null) {
    auto ctx = sched::IoContext();
    auto spec = flow::pipeline(ctx) | flow::pipeline_finish();
    (void)spec;
}

template <class T>
struct DummySrc final : public flow::Src<T> {
    std::optional<T> buffer{};
    T produce() override { return *std::exchange(buffer, std::nullopt); }
    bool src_ready() override { return buffer.has_value(); }

    void do_produce(T val) {
        buffer.emplace(std::move(val));
        flow::Src<T>::notify_src_ready();
    }
};

template <class T>
class DummySink final : public flow::Sink<T> {
 public:
    std::vector<T> produced{};
    flow::SinkSubscriber* subscriber{};
    void consume(T val) override { produced.push_back(std::move(val)); }

    bool sink_ready() override { return true; }
};

TEST(Construct, SrcSink) {
    auto src = DummySrc<int>();
    auto sink = DummySink<int>();
    auto ctx = sched::IoContext();

    auto spec = flow::pipeline(ctx) | src | sink | flow::pipeline_finish();
    (void)spec;
}

template <class T, class U>
class Proxy final : public flow::Sink<T>, public flow::Src<U> {
 public:
    explicit Proxy(std::function<U(T)> converter) : m_conv(converter) {}

    U produce() override { return *std::exchange(m_buf, std::nullopt); }
    bool src_ready() override { return m_buf.has_value(); }

    void consume(T val) override { m_buf.emplace(m_conv(std::move(val))); }
    bool sink_ready() override { return !m_buf.has_value(); }

 private:
    std::function<U(T)> m_conv;
    std::optional<U> m_buf{};
};

TEST(Construct, SrcProxySink) {
    auto src = DummySrc<int>();
    auto proxy = Proxy<int, float>([](int i) { return float(i); });
    auto sink = DummySink<float>();
    auto ctx = sched::IoContext();

    auto pipeline =
        flow::pipeline(ctx) | src | proxy | sink | flow::pipeline_finish();
}
