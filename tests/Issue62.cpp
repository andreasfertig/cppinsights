#define INSIGHTS_USE_TEMPLATE
template<typename... T>
int func(T... t) {
    return func(t...);
}
int _ = func(5, 5);
