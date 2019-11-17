// Source: https://twitter.com/bjorn_fahller/status/1039778723791335424
#define INSIGHTS_USE_TEMPLATE

// XXX: This does not compile after transformation, as T inside of the generated lambda has no meaning. And the closure
// type is infact no template. Instantiations of func on the other hand will compile as there T is replace with a
// concrete type. One solution could be to not expand the lambda here and only do it for the instantiations.
template <typename T>
constexpr auto func = [](auto x){ return T(x);};

template <typename T>
constexpr auto funcBraced = [](auto x){ return T{x};};

double f(int x)
{
    return func<double>(x);
}

double fBraced(int x)
{
    return funcBraced<int>(x);
}
