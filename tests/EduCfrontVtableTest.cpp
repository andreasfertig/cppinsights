// cmdlineinsights:-edu-show-cfront

#include <cstdio>

struct A {
  double       md;
  virtual void Fun() { puts("fun a"); }
};

struct B : A {
  int  mX{5};
  void Fun() { printf("fun b: %d\n", mX); }

  virtual void Other() {}
};

int main()
{
  B b{};

  b.Fun();

  A* a{&b};
  a->Fun();
}

