// Source: https://twitter.com/bjorn_fahller/status/1039778723791335424
#define INSIGHTS_USE_TEMPLATE

template <typename T>
constexpr auto func = [](auto x){ return T(x);};

double f(int x)
{
    return func<double>(x);
}
