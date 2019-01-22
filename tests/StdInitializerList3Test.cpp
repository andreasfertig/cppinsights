// cmdlineinsights:-edu-show-initlist
#include <initializer_list>

struct Foo
{
    Foo(std::initializer_list<int> l) {}
};

class Test
{
public:
    Test()
    {}

    Foo v{1,2,3};
};


class Best
{
public:
    Best()
    : v{1,2}
    {}

    Foo v{1,2,3};
};

class WestE
{
    Foo v{1,2,3};
public:
    WestE()
    : v{1,2}
    {}

};
