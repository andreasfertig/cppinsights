#include <optional>

struct A {};

struct Test
{
    Test(const A &);
    Test(A &&);
};

std::optional<Test> foo()
{
    A a;
    return a;
}
