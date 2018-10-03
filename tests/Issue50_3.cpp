#define INSIGHTS_USE_TEMPLATE

template<class F>
void foo(F&& f)
{
}

using F = int && (int&&);

int&& f(int&& i)
{
    return static_cast<int&&>(i);
}

int main()
{
    F& rf = f;
    foo(rf);
    return 0;
}
