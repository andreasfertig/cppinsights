// cmdlineinsights:-edu-show-lifetime

struct Outer
{
    Outer() {}
};

struct Inner
{
    Inner(Outer) {}
};

auto Test()
{
    return Inner{Outer{}};
}
