#pragma once

#include <concepts>
#include <type_traits>

namespace plai::flow {

class SrcSubscriber {
 public:
    virtual ~SrcSubscriber() = default;
    virtual void src_data_available() = 0;
};

template <class T>
class Src {
 public:
    using produced_type = T;
    virtual ~Src() = default;

    virtual T produce() = 0;
    virtual bool data_available() = 0;

    virtual void on_data_available(SrcSubscriber* sub) = 0;
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
