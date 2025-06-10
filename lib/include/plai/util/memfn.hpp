#pragma once

#include <utility>

namespace plai {

template <class C, class R, class... Ps>
constexpr auto memfn(C* ptr, R (C::*mthd)(Ps...)) noexcept {
    return
        [ptr, mthd](Ps... ps) { return (ptr->*mthd)(std::forward<Ps>(ps)...); };
}

}  // namespace plai
