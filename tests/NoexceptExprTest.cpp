#include <cstdio>

void Foo() noexcept(false)
{
}

void Bar() noexcept(true)
{
}

void BarFoo() noexcept
{
}

void FooFoo() noexcept(noexcept(Foo()))
{
}


int main()
{
    printf("Foo: %d\n", noexcept(Foo()));
    printf("Bar: %d\n", noexcept(Bar()));
}
