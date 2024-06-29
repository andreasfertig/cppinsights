// cmdlineinsights:-edu-show-cfront

#include <cstdio>

struct A {
  double md;
  virtual ~A() { puts("dtor"); }

  virtual void Fun() { puts("fun a"); }
};

struct B : A {
  int  mX{5};
  void Fun() { printf("fun b: %d\n", mX); }

  virtual void Other() {}
};

struct C : B {
  int  mB{8};
  void Fun() { printf("fun c: %d\n", mB); }

  virtual void Other() {}
};

int main()
{
  C b{};

  b.Fun();

  A* a{&b};
  a->Fun();
}

