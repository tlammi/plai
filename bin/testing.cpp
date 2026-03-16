#include <exec/async_scope.hpp>
#include <exec/ensure_started.hpp>
#include <exec/reschedule.hpp>
#include <exec/single_thread_context.hpp>
#include <exec/start_detached.hpp>
#include <plai/ex/scheduler.hpp>
#include <plai/ex/visit_on.hpp>
#include <print>
#include <stdexec/execution.hpp>

using namespace std::literals;

constexpr auto print_tid() {
    return stdexec::then(
        [] { std::println("tid: {}", std::this_thread::get_id()); });
}

constexpr plai::ex::AnySenderOf<> foo(auto aux_sched, auto main_sched) {
    return stdexec::schedule(main_sched) | print_tid() |
           stdexec::continues_on(aux_sched) | print_tid() |
           stdexec::continues_on(main_sched);
}

constexpr auto eager(int i, auto sched) {
    namespace ex = exec;
    using namespace stdexec;

    return get_scheduler() | let_value([=](auto orig_sched) {
               return ex::ensure_started(schedule(sched) | then([=]() {
                                             std::println(
                                                 "thread: {}",
                                                 std::this_thread::get_id());
                                             return i;
                                         })) |
                      continues_on(orig_sched);
           });
}

int main() {
    namespace ex = exec;
    using namespace stdexec;
    namespace ex = exec;
    namespace se = stdexec;
    auto loop = stdexec::run_loop();
    auto other = stdexec::run_loop();
    auto worker = std::jthread([&](std::stop_token st) {
        auto cb = std::stop_callback(st, [&] { other.finish(); });
        other.run();
    });
    std::println("{}", std::this_thread::get_id());
    sync_wait(just() |
              plai::ex::visit_on(
                  other.get_scheduler(),
                  [] { std::println("{}", std::this_thread::get_id()); }) |
              print_tid());
    std::println("{}", std::this_thread::get_id());

    std::println("finish");
    loop.finish();
    std::println("run");
    loop.run();
    std::println("request_stop");
    worker.request_stop();
    std::println("join");
    worker.join();
    std::println("run2");
    loop.run();
    std::println("done");
}
