#pragma once

#include <atomic>
#include <concepts>
#include <optional>
#include <type_traits>

namespace plai::flow {

class SrcSubscriber {
 public:
    virtual ~SrcSubscriber() = default;
    virtual void src_ready() = 0;
};

template <class T>
class Src {
 public:
    using produced_type = T;
    virtual ~Src() = default;

    virtual T produce() = 0;

    virtual bool src_ready() = 0;

    void set_subscriber(SrcSubscriber* sub) noexcept { m_sub = sub; }

 protected:
    void notify_src_ready() {
        auto ptr = m_sub.load();
        if (ptr) ptr->src_ready();
    }

 private:
    std::atomic<SrcSubscriber*> m_sub{};
};

namespace src_detail {

template <class T, class = void>
struct has_produced_type : std::false_type {};

template <class T>
struct has_produced_type<T, std::void_t<typename T::produced_type>>
    : std::true_type {};
}  // namespace src_detail

template <class T>
concept src_type = src_detail::has_produced_type<T>::value &&
                   std::derived_from<T, Src<typename T::produced_type>>;

}  // namespace plai::flow
