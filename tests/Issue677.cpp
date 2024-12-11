
template<template<class, class> class C, class T>
using wrapper = T;

template<class T>
struct helper {
  template<class U>
  using type = U;
};

template<class T>
using outer = wrapper<helper<T>::template type, int>;

