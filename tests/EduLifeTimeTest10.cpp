// cmdlineinsights:-edu-show-lifetime

#include <initializer_list>

struct Vector
{
    Vector(std::initializer_list<const char*>) {}
};

void Fun(Vector) {}

Vector createStrings()
{
    return {"This", "is", "a", "vector", "of", "strings"};
}

Vector createStrings2()
{
    Vector v{"This", "is", "a", "vector", "of", "strings"};

    return v;
}

void test()
{
    Fun({"This", "is", "a", "vector", "of", "strings"});
}
