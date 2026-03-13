#pragma once

#include <plai/ex/scheduler.hpp>

namespace plai::mods {

class Context {
 public:
    /**
     * \brief Get the main scheduler
     *
     * This is the main scheduler of the program, guaranteed to be executed from
     * the main thread. The scheduler is single threaded.
     * */
    virtual ex::AnyScheduler scheduler() = 0;

    /**
     * \brief Reguest process to stop
     *
     * This might happen e.g. due to user input on GUI.
     * */
    virtual void request_stop() = 0;

 protected:
    constexpr ~Context() noexcept = default;
};

/**
 * \brief Module
 *
 * Module is an unit of execution. Generally modules get references to other
 * modules or objects in their constructors and start execution in start()
 * method.
 * */
class Module {
 public:
    constexpr virtual ~Module() = default;

    virtual void start(Context& ctx) = 0;

    /**
     * \brief Inform module to stop
     * */
    virtual void stop() = 0;
};
}  // namespace plai::mods
