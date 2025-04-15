#pragma once

#include <functional>
#include <memory>
#include <span>

namespace plai::os {

using Signal = int;
class SignalListener {
 public:
    SignalListener(std::span<const Signal> mask, std::function<bool()> on_sig);

    SignalListener(const SignalListener&) = delete;
    SignalListener& operator=(const SignalListener&) = delete;
    SignalListener(SignalListener&&) noexcept = default;
    SignalListener& operator=(SignalListener&&) noexcept = default;
    ~SignalListener();

 private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
}  // namespace plai::os
