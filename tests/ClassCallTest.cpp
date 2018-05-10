#include <cstdio>

class Foo {
public:
  Foo(int foo) : mFoo(foo) {}

  int Mul(int f) { return f * GetFoo(); }

protected:
  int GetFoo() const { return mFoo; }

private:
  int mFoo;
};

class Bar : public Foo {
public:
  Bar(int foo, int bar) 
  : Foo(foo), mBar(bar) {}

  int Sum() { return mBar + GetFoo(); }

private:
  int mBar;
};

int main() {
  Foo foo(10);
  Bar bar(10, 5);

  printf( "%d %d %d\n", foo.Mul(2), 
          bar.Sum(), bar.Mul(3) );
}

