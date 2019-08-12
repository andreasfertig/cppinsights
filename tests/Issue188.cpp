#include <bits/c++config.h>
#include <bits/move.h>

#include <cstddef>

#define _GLIBCXX_TUPLE 1

namespace std {
_GLIBCXX_BEGIN_NAMESPACE_VERSION

template <std::size_t _Idx, typename _Head, bool _IsEmptyNotFinal>
struct _Head_base;

template <std::size_t _Idx, typename _Head>
struct _Head_base<_Idx, _Head, true> : public _Head {
  constexpr _Head_base() : _Head() {}
  constexpr _Head_base(const _Head& __h) : _Head(__h) {}
  constexpr _Head_base(const _Head_base&) = default;
};

template <std::size_t _Idx, typename _Head>
struct _Head_base<_Idx, _Head, false> {
  constexpr _Head_base() : _M_head_impl() {}
  constexpr _Head_base(const _Head& __h) : _M_head_impl(__h) {}
  constexpr _Head_base(const _Head_base&) = default;

  _Head _M_head_impl;
};
  
template <std::size_t _Idx, typename... _Elements>
struct _Tuple_impl;

template <std::size_t _Idx>
struct _Tuple_impl<_Idx> {
  template <std::size_t, typename...>
  friend class _Tuple_impl;

  _Tuple_impl() = default;
};

template <typename _Tp>
struct __is_empty_non_tuple : is_empty<_Tp> {};

template <typename _El0, typename... _El>
struct __is_empty_non_tuple<tuple<_El0, _El...>> : false_type {};

template <typename _Tp>
using __empty_not_final = typename conditional<__is_final(_Tp), false_type,
                                               __is_empty_non_tuple<_Tp>>::type;

template <std::size_t _Idx, typename _Head, typename... _Tail>
struct _Tuple_impl<_Idx, _Head, _Tail...>
    : public _Tuple_impl<_Idx + 1, _Tail...>,
      private _Head_base<_Idx, _Head, __empty_not_final<_Head>::value> {

  typedef _Tuple_impl<_Idx + 1, _Tail...> _Inherited;
  typedef _Head_base<_Idx, _Head, __empty_not_final<_Head>::value> _Base;

  constexpr _Tuple_impl() : _Inherited(), _Base() {}

  explicit constexpr _Tuple_impl(const _Head& __head, const _Tail&... __tail)
      : _Inherited(__tail...), _Base(__head) {}

  template <typename... _UElements>
  constexpr _Tuple_impl(const _Tuple_impl<_Idx, _UElements...>& __in)
      : _Inherited(_Tuple_impl<_Idx, _UElements...>::_M_tail(__in)),
        _Base(_Tuple_impl<_Idx, _UElements...>::_M_head(__in)) {}
};

_GLIBCXX_END_NAMESPACE_VERSION
}  // namespace std

std::_Tuple_impl<1, short, long> one_s_l;

