// cmdlineinsights:-edu-show-cfront

template<typename BT>
struct Foo
{
    BT raw;

    constexpr Foo() = default;

    constexpr Foo(BT v)
    : BT(v)
    {
    }
};

int main()
{
    Foo<int> f{};
}
