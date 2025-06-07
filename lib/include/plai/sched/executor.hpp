#pragma once

#include <boost/asio.hpp>

namespace plai::sched {
using Executor = boost::asio::any_io_executor;

template <class... Ts>
auto post(Ts&&... ts) {
    return boost::asio::post(std::forward<Ts>(ts)...);
}

using IoContext = boost::asio::io_context;
}  // namespace plai::sched
