template <template <typename> class... Templates>
struct template_tuple {};
template <typename T>
struct identity {};
template <template <typename> class... Templates>
template_tuple<Templates...> f7() {}

void foo2() {
  f7<identity>();
}


  template<typename> class A { };
  template<template<typename> class ...> class B { };
  int foo();

  template<typename> class testType { };

  template<int fp(void)> class testDecl { };

  template<int> class testIntegral { };

  template<template<typename> class> class testTemplate { };
  template class testTemplate<A>;

  template<template<typename> class ...T> class C {
    B<T...> testTemplateExpansion;
  };

  template<int, int = 0> class testExpr{};

  template<int, int ...> class testPack { };


template<typename T>
struct Test {};

int main() {


    C<Test, Test> te;

    testType<int> type;
    
    testDecl<foo> a{};
    testDecl<nullptr> b{};

    testIntegral<2> integral;

    testExpr<2*2, 1> expr;

    testPack<0, 1, 2, 4> pack;
}

