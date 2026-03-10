// cmdlineinsights:-edu-show-cfront

#include <cstdio>

class Fruit {
public:
  Fruit()
  {
    Print();
  }

  virtual ~Fruit() { Print(); }

  virtual void Print() const { puts("Base"); }
};

class Apple : public Fruit {
public:
  Apple()
  : Fruit{}
  {}

  virtual ~Apple() override { Print(); }
  
  void Print() const override { puts("Apple"); }
};

int main()
{
  Apple x{};
}

