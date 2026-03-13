#pragma once

#include <stdexec/execution.hpp>

namespace plai::ex {

template <class... Ts>
using CompletionSignature =
    stdexec::completion_signatures<stdexec::set_value_t(Ts...),
                                   stdexec::set_error_t(std::exception_ptr),
                                   stdexec::set_stopped_t()>;
}
