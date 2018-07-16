template<typename T>
struct Foo {
  Foo(char x, int y) : _x{x}, _y(y) {}
  Foo(int y) : Foo('a', y) {}

  char _x;
  int _y;
};


template<typename T>
struct Bar : Foo<T>
{
    Bar(int x) : Foo<T>(x, 2) {}
};

int main()
{
    Foo<int> f{1,2};
    Foo<int> f2{1};

    Bar<char> b{1};
}
