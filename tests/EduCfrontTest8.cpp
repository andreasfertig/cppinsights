// cmdlineinsights:-edu-show-cfront

#include <cstdio>
#include <cstdlib>
#include <new>

class Apple
{
    char mX{};
    int  mY{};

public:
    Apple(int x, int y)
    : mX(x)
    , mY{y}
    {
        printf("Apple\n");
    }

    ~Apple() { printf("~Apple\n"); }
};

int main()
{
    Apple f{2, 3};

    alignas(Apple) char buffer[sizeof(Apple)]{};  // look here
    Apple*              b = new(buffer) Apple{4, 5};

    printf("Created\n");
    b->~Apple();  // look here

    char data[5]{4, 5, 6};
}
