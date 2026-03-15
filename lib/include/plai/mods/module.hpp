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
     * \brief Request program to exit gracefully
     * */
    virtual void request_finish() = 0;

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
     * \brief Gracefully stop the module
     * */
    virtual void finish() = 0;

    /**
     * \brief Stop the module ungracefully
     * */
    virtual void stop() = 0;
};
}  // namespace plai::mods
