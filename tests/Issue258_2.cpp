#include <cstdio>

class NonMoveable
{
    int mX;
public:
    NonMoveable(int x) : mX{x} {}
    

    NonMoveable(const NonMoveable&) = delete;
    NonMoveable& operator=(const NonMoveable&) = delete;

    NonMoveable(NonMoveable&&) = default;
    NonMoveable& operator=(NonMoveable&&) = default;

    int Get() const { return mX; }
};

int main()
{
    auto x = NonMoveable(42);
    auto lambda = [x = static_cast<NonMoveable&&>(x)]() { printf("%d\n", x.Get()); };
}

