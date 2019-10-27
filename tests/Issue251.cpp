#include <utility>

class Foo
{
};
static_assert(noexcept(Foo( Foo() )), "");
static_assert(not noexcept( new Foo() ), "");

int main()
{
  Foo f;
// Foo ff = f;
//  Foo fff = std::move(f);
}
