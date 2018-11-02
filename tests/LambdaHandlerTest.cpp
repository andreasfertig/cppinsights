#include <cstdio>

class Bar
{
    Bar()
    : a{1}
    {
        auto x = [this]() { printf("%d\n", this->a); };
    }

    int a;

};

int main()
{
    int foo = 2;
    int b = 4;

    auto lambda = [&foo]() { printf( "%d\n", foo); };

    lambda();

    auto lambda2 = []() { printf( "%d\n", 1); };

    lambda2();

    auto lambda3 = [&foo]() mutable { printf( "%d\n", ++foo);  return foo; };

    lambda3();

    auto lambda4 = [&foo](int x)  { printf( "%d\n", foo+x);  return foo+x; };

    auto lambda5 = [&foo](const int x)  { printf( "%d\n", foo+x);  return foo+x; };

    auto lambda6 = [&foo](int& x, int b)  { printf( "%d\n", foo+x);  return foo+x+b; };

    auto lambda7 = [&foo](int& x, auto b)  { printf( "%d\n", foo+x);  return foo+x+b; };
    lambda7(foo, 2);

    auto lambda71 = [&foo](int& x, const auto& b)  { printf( "%d\n", foo+x);  return foo+x+b; };

    auto lambda73 = [&foo](int& x, const auto& b, const auto c)  { printf( "%d\n", foo+x);  return foo+x+b+c; };
    lambda73(foo, b, 44);

    auto lambda8 = [foo, b](int& x, int bb)  { printf( "%d\n", foo+x);  return foo+x+bb; };

    auto lambda9 = [foo, b](int& x, int bb) noexcept { printf( "%d\n", foo+x);  return foo+x+bb; };
}
