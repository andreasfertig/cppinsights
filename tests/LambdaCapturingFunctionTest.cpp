#include <cstdio>

void Dummy(int x) {
    printf("%d\n", x);
}

int main()
{
    using D = void(*)(int);

    D d = Dummy;

    d(2);

    auto l = [&]()
    {
        d(3);
    };

    l();
}
