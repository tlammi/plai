#include <cassert>
#include <csignal>
#include <cstring>
#include <plai/exceptions.hpp>
#include <plai/logs/logs.hpp>
#include <plai/os/signal.hpp>
#include <thread>

namespace plai::os {
namespace {
sigset_t build_sigset(std::span<const Signal> mask) {
    sigset_t out{};
    auto res = sigemptyset(&out);
    if (res) throw ValueError(format("sigemptyset: {}", strerror(errno)));
    for (auto s : mask) {
        res = sigaddset(&out, s);
        if (res)
            throw ValueError(format("sigaddset: {}", strerror(errno)));
    }
    return out;
}

std::jthread launch_worker(std::span<const Signal> mask,
                           std::function<bool()> on_sig) {
    auto set = build_sigset(mask);
    int res = pthread_sigmask(SIG_BLOCK, &set, nullptr);
    if (res)
        throw ValueError(format("pthread_sigmask: {}", strerror(errno)));
    return std::jthread([set, on_sig = std::move(on_sig)]() {
        while (true) {
            int sig{};
            PLAI_TRACE("listening for signals");
            int res = sigwait(&set, &sig);
            if (res)
                throw ValueError(format("sigwait: {}", strerror(errno)));
            PLAI_TRACE("Received signal {}", sig);
            if (on_sig()) {
                PLAI_TRACE("Terminating signal listener");
                return;
            }
        }
    });
}

}  // namespace

class SignalListener::Impl {
 public:
    Impl(std::span<const Signal> mask, std::function<bool()> on_sig)
        : m_waiter(launch_worker(mask, std::move(on_sig))) {}

 private:
    std::jthread m_waiter{};
};

SignalListener::SignalListener(std::span<const Signal> mask,
                               std::function<bool()> on_sig)
    : m_impl(std::make_unique<Impl>(mask, std::move(on_sig))) {}

SignalListener::~SignalListener() = default;

}  // namespace plai::os
