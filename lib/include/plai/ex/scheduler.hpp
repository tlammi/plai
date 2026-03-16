#pragma once

#include <exec/any_sender_of.hpp>
#include <plai/ex/completion_signatures.hpp>
#include <stdexec/execution.hpp>

namespace plai::ex {

using AnyScheduler = exec::any_receiver_ref<
    CompletionSignature<>>::any_sender<>::any_scheduler<>;

template <class... Ts>
using AnySenderOf = typename exec::any_receiver_ref<
    CompletionSignature<Ts...>>::template any_sender<>;

}  // namespace plai::ex
