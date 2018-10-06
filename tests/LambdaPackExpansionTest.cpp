#define INSIGHTS_USE_TEMPLATE

template<class... Args>
int g(Args ...)
{
    return 1;
}

template<class... Args>
void f(Args... args) {
    auto lm = [&] { return g(args...); };
    lm();
}

template<class... Args>
void f2(Args... args) {
    auto lm = [&, args...] { return g(args...); };
    lm();
}

int main()
{
    f(1, 2, 3 , 4, 5);
    f2(1, 2, 3 , 4, 5);
}
