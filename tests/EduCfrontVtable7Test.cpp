// cmdlineinsights:-edu-show-cfront

#include <cstdio>

struct Fruit {
  double md;
  virtual ~Fruit() { puts("~Fruit"); }
  virtual void Fun() { puts("Fruit's Fun"); }
};

struct Apple : Fruit {
  int  mX{5};
  void Fun() override { printf("Apple's Fun: %d\n", mX); }
};

struct PinkLady : Apple {
  int  mApple{8};
  void Fun() override { printf("Pink Ladies Fun: %d\n", mApple); }
};

int main()
{
  PinkLady delicious{};
  delicious.Fun();

  Fruit* f{static_cast<Fruit*>(&delicious)};
  f->Fun();
}

