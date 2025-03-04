// cmdlineinsights:-edu-show-cfront

#include <stdio.h>

struct Fruit {
  double mWeight{};
  virtual ~Fruit() { puts("~Fruit"); }
  virtual void Print() { puts("Fruit's Print"); }
};

struct Apple : Fruit {
  int mRipeGrade{5};
  void Print() override { printf("Apple's Print: %d\n", mRipeGrade); }
};

struct PinkLady : Apple {
  int mColorGrade{8};
  void Print() override { printf("Pink Ladies Print: %d\n", mColorGrade); }
};

int main() {
  PinkLady delicious{};
  delicious.Print();
  Fruit *f{static_cast<Fruit *>(&delicious)};
  f->Print();
}
