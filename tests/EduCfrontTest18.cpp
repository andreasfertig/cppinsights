// cmdlineinsights:-edu-show-cfront

class Foo
{
public:
    Foo(int foo)
    : mFoo(foo)
    {
    }

private:
    int mFoo;
};

class Bar : public Foo
{
public:
    Bar(int foo, int bar)
    : Foo(foo)
    {
    }

private:
    int mBar;
};
