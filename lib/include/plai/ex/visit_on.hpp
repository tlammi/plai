#pragma once

#include <stdexec/execution.hpp>

namespace plai::ex {

template <stdexec::scheduler Sched, class Fn>
class visit_on_t final
    : public stdexec::sender_adaptor_closure<visit_on_t<Sched, Fn>> {
    Sched m_sched;
    Fn m_fn;

 public:
    constexpr explicit visit_on_t(Sched s, Fn fn) noexcept
        : m_sched(std::move(s)), m_fn(std::move(fn)) {}

    template <class Self, class T>
    constexpr auto operator()(this Self&& self, T&& t) noexcept {
        using namespace stdexec;
        return get_scheduler() |
               let_value([sched = std::forward<Self>(self).m_sched,
                          fn = std::forward<Self>(self).m_fn,
                          t = std::forward<T>(t)](auto&& orig_sched) mutable {
                   return t | on(std::move(sched), then(std::move(fn))) |
                          continues_on(orig_sched);
               });
    }
};

constexpr auto visit_on(auto&&... args) noexcept {
    return visit_on_t(std::forward<decltype(args)>(args)...);
}

}  // namespace plai::ex
