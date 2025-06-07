#pragma once

#include <type_traits>

namespace plai::flow {

template <class T>
class Src;
template <class T>
class Sink;
namespace concepts {
namespace detail {
template <class T>
struct is_src : std::false_type {};

template <class T>
struct is_src<Src<T>> : std::true_type {};

template <class T>
struct is_sink : std::false_type {};

template <class T>
struct is_sink<Sink<T>> : std::true_type {};

}  // namespace detail

template <class T>
concept src_type = detail::is_src<T>::value;

template <class T>
concept sink_type = detail::is_sink<T>::value;
}  // namespace concepts
}  // namespace plai::flow
