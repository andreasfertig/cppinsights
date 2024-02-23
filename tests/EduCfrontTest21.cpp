// cmdlineinsights:-edu-show-cfront

#define INSIGHTS_USE_TEMPLATE

template<typename T>
struct Foo
{
    Foo(char x, int y)
    : _x{x}
    , _y(y)
    {
    }
    Foo(int y) {}

    char _x;
    int  _y;
};
