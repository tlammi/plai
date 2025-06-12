#pragma once

#include <tuple>
#include <type_traits>

namespace plai {

template <class... Ts>
struct pack {};

template <auto... Vs>
struct value_pack {};

template <class T, T... Vs>
struct typed_value_pack {};

template <class Pack>
struct pack_size;

template <class... Ts>
struct pack_size<pack<Ts...>> {
    static constexpr auto value = sizeof...(Ts);
};

template <class Pack>
constexpr auto pack_size_v = pack_size<Pack>::value;

template <class T>
struct prototype_of : prototype_of<decltype(&T::operator())> {};

template <class R, class... Ps>
struct prototype_of<R(Ps...)> {
    using type = R(Ps...);
};

template <class R, class... Ps>
struct prototype_of<R (&)(Ps...)> {
    using type = R(Ps...);
};

template <class R, class... Ps>
struct prototype_of<R (*)(Ps...)> {
    using type = R(Ps...);
};

template <class R, class C, class... Ps>
struct prototype_of<R (C::*)(Ps...)> {
    using type = R(Ps...);
};

template <class R, class C, class... Ps>
struct prototype_of<R (C::*)(Ps...) const> {
    using type = R(Ps...);
};

template <class T>
using prototype_of_t = typename prototype_of<T>::type;

namespace tt_detail {
template <class T>
struct invoke_result;

template <class R, class... Ps>
struct invoke_result<R(Ps...)> {
    using type = R;
};

template <class T>
struct invoke_params;

template <class R, class... Ps>
struct invoke_params<R(Ps...)> {
    using type = pack<Ps...>;
};

}  // namespace tt_detail

template <class T>
using invoke_result_t =
    typename tt_detail::invoke_result<prototype_of_t<T>>::type;

template <class T>
using invoke_params_t =
    typename tt_detail::invoke_params<prototype_of_t<T>>::type;

template <class T, class... Ts>
struct first_type {
    using type = T;
};

template <class... Ts>
using first_type_t = typename first_type<Ts...>::type;

template <class Pack>
struct first_pack_type;

template <class... Ts>
struct first_pack_type<pack<Ts...>> : first_type<Ts...> {};

template <class P>
using first_pack_type_t = typename first_pack_type<P>::type;

template <class T, class... Ts>
struct last_type : public last_type<Ts...> {};

template <class T>
struct last_type<T> {
    using type = T;
};

template <class... Ts>
using last_type_t = typename last_type<Ts...>::type;

template <class Pack>
struct pack_to_tuple;

template <class... Ts>
struct pack_to_tuple<pack<Ts...>> {
    using type = std::tuple<Ts...>;
};

template <class T>
using pack_to_tuple_t = typename pack_to_tuple<T>::type;

}  // namespace plai
