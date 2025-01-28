#include <new>

namespace Test {
  template<typename T>
  struct Apple
  {
    T x;
  };

  template<typename T>
  void destroy(T* ptr) {
    ptr->T::~T();
  }
}

namespace West
{
  int y;
}

int main()
{
    char buffer[sizeof(Test::Apple<int>)];

    auto f = new (&buffer) Test::Apple<int>;

    f->Test::Apple<int>::~Apple<int>();
    f->~Apple();
    
    Test::destroy(f);

    int x;
    Test::destroy(&x);

    Test::destroy(&West::y);
}
