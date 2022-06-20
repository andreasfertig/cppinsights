// cmdline:-std=c++20

#define INSIGHTS_USE_TEMPLATE

namespace A {
    template<class F = decltype([]() -> bool { return true; })>
    bool test(F f = {})
    {
        return f();
    }

    bool call() { return test(); }

}  // namespace A

int main() { return A::call(); }

