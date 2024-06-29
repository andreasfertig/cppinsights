// cmdlineinsights:-edu-show-cfront

#include <cstdio>

class Base {
public:
  virtual ~Base() { puts("~Base"); }
};

class Derived : public Base {
public:
  ~Derived() { puts("~Derived"); }
};

class BaseNonVirtual {
public:
  ~BaseNonVirtual() { puts("~BaseNonVirtual"); }
};

class DerivedNonVirtual : public BaseNonVirtual {
public:
  ~DerivedNonVirtual() { puts("~DerivedNonVirtual"); }
};

int main()
{
  Derived           d{};
  DerivedNonVirtual nvd;
}

