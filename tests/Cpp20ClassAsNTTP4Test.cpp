// cmdline:-std=c++20

#define INSIGHTS_USE_TEMPLATE 1

struct A { int i; };

template<A a>
class Foo
{
public:
    void Func();
};


template<>
void Foo<A{2}>::Func()
{
}

template<>
void Foo<A{3}>::Func()
{
}

int main()
{
    Foo<A{2}> f;
    Foo<A{3}> fc;
}
