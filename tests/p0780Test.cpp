// cmdline:-std=c++20

// Example from: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0780r2.html
// Allow pack expansion in lambda init-capture
#include <functional>
#include <utility>

void Func(int, int, int, int) {}

template<class F, class... Args>
auto delay_invoke(F f, Args... args)
{
    return [f = std::move(f), ... args = std::move(args)]() -> decltype(auto) { return std::invoke(f, args...); };
}

int main()
{
    delay_invoke(Func, 2, 3, 4, 5);
}

