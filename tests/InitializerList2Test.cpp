#include <initializer_list>

class Foo
{
public:
    Foo(std::initializer_list<int> x, const bool b=false, const int z=2){}
};

int main()
{
  Foo f{1};
  Foo f2{1, true};
  Foo f3{1, true, 4};
}
