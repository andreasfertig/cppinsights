// cmdline:-std=c++20

// Example from: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0780r2.html
// Allow pack expansion in lambda init-capture
#include <functional>
#include <utility>

#define INSIGHTS_USE_TEMPLATE

void Func(int, int, int, int) {}

template<class F, class... Args>
auto delay_invoke(F f, Args... args)
{
    return [f = std::move(f), ... targs = std::move(args)]() -> decltype(auto) { return std::invoke(f, targs...); };
}

int main()
{
    auto df = delay_invoke(Func, 2, 3, 4, 5);

    df();
}

