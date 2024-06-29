// cmdlineinsights:-edu-show-cfront

#include <cstdio>

struct A {
  double md;
};

struct B {
  int  mX{5};

  virtual void Fun() { puts("B::Fun"); }
};

struct C : A, B {
  int  mB{8};
  void Fun() { puts("C::Fun"); }
};

int main()
{
  C c{};

  c.Fun();

  B* b{&c};
  b->Fun();
}

