#include <type_traits>
#include <utility>

template <typename T, T v>
struct typed_constant
{
  using type = T;
  static constexpr T value{v};
  constexpr operator type() const { return v;}
};

template<auto V>
using constant = typed_constant<std::remove_cv_t<decltype(V)>, V>;

template<auto V>
inline constexpr auto constant_v = constant<V>{};

template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1+v2)> operator+(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1-v2)> operator-(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1/v2)> operator/(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1*v2)> operator*(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1%v2)> operator%(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1==v2)> operator==(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1!=v2)> operator!=(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1<v2)> operator<(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1<=v2)> operator<=(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1>v2)> operator>(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1>=v2)> operator>=(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1|v2)> operator|(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1&v2)> operator&(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1^v2)> operator^(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1&&v2)> operator&&(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}
template <typename T1, T1 v1, typename T2, T2 v2>
constexpr constant<(v1||v2)> operator||(typed_constant<T1,v1>,typed_constant<T2,v2>) { return {};}

template <typename T1, auto v1>
constexpr constant<!v1> operator!(typed_constant<T1,v1>) { return {};}
template <typename T1, T1 v1>
constexpr constant<(~v1)> operator~(typed_constant<T1,v1>) { return {};}


namespace detail
{

template <typename T, typename = void>
struct has_continuation
{
  static constexpr bool value = false;
};

template <typename T>
struct has_continuation<T, std::void_t<typename T::continuation>>
{
  static constexpr bool value = true;
};

template <typename T, bool = has_continuation<T>::value>
struct to_
{
  using type = T;
};

template <typename T>
struct to_<T, true>
{
  using type = typename T::template to<typename T::continuation>;
};


template <typename T>
using to = typename to_<T>::type;

}

template<typename...>
struct list {};

template<template<typename ...> class C>
struct make {
  template<typename ... Ts>
  using result = C<Ts...>;
};

template <typename F, typename T>
using apply_one = typename F::template result<T>;

struct identity {
  template<typename T>
  using result = T;
};

template <template <typename ...> class T, typename ... Ts>
struct bind_front {
  template <typename ... Vs>
  struct helper {
    using type = T<Ts..., Vs...>;
  };
  template <typename ... Vs>
  using result = typename helper<Vs...>::type;
};

template<template <typename...> class T>
struct from_type {
  using continuation = identity;
  template <typename C = continuation>
  struct to {
    using TO = detail::to<C>;
    template<typename ... Vs>
    struct helper {
      using type = apply_one<TO, typename T<Vs...>::type>;
    };
    template<typename ... Vs>
    using result = typename helper<Vs...>::type;
  };
};

template <template <typename ...> class T>
struct from_value
{
  using continuation = identity;
  template <typename C = continuation>
  struct to {
    using TO = detail::to<C>;
    template<typename ... Vs>
    using result = apply_one<TO, constant<T<Vs...>::value>>;
  };
};


template <typename ...>
struct compose;

template <>
struct compose<>
{
  using continuation = identity;
  template <typename C = continuation>
  struct to
  {
    using T = detail::to<C>;
    template <typename ... Ts>
    using result = typename T::template result<Ts...>;
  };
};

template <typename F, typename ... Fs>
struct compose<F, Fs...>
{
  using continuation = typename F::continuation;
  template <typename C = continuation>
  struct to
  {
    template <typename ... Ts>
    using result = typename compose<Fs...>::template to<typename F::template to<C>>::template result<Ts...>;
  };
};

struct add_const
{
  using continuation = identity;
  template <typename C = continuation>
  struct to
  {
    template <typename T>
    struct inner {
      using type = apply_one<C, const T>;
    };
    template <typename ...T>
    using result = typename inner<T...>::type;
  };
};

struct add_pointer
{
  using continuation = identity;
  template <typename C = continuation>
  struct to
  {
    template <typename T>
    struct inner {
      using type = apply_one<C, T*>;
    };
    template <typename ...T>
    using result = typename inner<T...>::type;
  };
};

template <typename T>
struct wrapped {};

struct wrap
{
  template <typename T>
  using result = wrapped<T>;
};

template <typename F, typename ... Ts>
using apply = typename F::template to<>::template result<Ts...>;
template <typename F, typename C, typename ... Ts>
using apply_to = typename F::template to<C>::template result<Ts...>;

static_assert(std::is_same_v<apply<compose<add_pointer>, int>, int*>);
static_assert(std::is_same_v<apply<compose<add_pointer, add_const>,int>, const int*>);
static_assert(std::is_same_v<apply<compose<add_const, add_pointer>,int>, int*const>);
static_assert(std::is_same_v<apply_to<compose<add_pointer>, wrap, int>, wrapped<int*>>);
static_assert(std::is_same_v<apply_to<compose<add_pointer, add_const>,wrap, int>, wrapped<const int*>>);

