#pragma once

#include <stdexec/execution.hpp>
namespace plai::ex {

struct yield_sender {
    using sender_concept = stdexec::sender_t;
    using completion_signatures =
        stdexec::completion_signatures<stdexec::set_value_t(),
                                       stdexec::set_stopped_t()>;

    template <class Receiver>
    struct operation_state {
        Receiver rcvr;

        void start() & noexcept {
            if (stdexec::get_stop_token(rcvr).stop_requested()) {
                stdexec::set_stopped(static_cast<Receiver&&>(rcvr));
            } else {
                stdexec::set_value(static_cast<Receiver&&>(rcvr));
            }
        }
    };

    template <class Receiver>
    auto connect(Receiver rcvr) const -> operation_state<Receiver> {
        return {static_cast<Receiver&&>(rcvr)};
    }
};

constexpr auto yield() noexcept { return yield_sender(); }
}  // namespace plai::ex
