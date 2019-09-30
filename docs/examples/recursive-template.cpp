#include <cstdio>
#include <vector>

template<int n>
struct A
{
    static const auto value = A<n - 1>::value + n;
};

template<>
struct A<1>
{
    static const auto value = 1;
};

int main()
{
    printf("c=%c\n", A<5>::value);
}
