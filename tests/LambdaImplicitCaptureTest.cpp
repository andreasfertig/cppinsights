#include <cstdio>

int main() 
{
    int x = 22;

    auto z = [x]() mutable { ++x; return x; } ();

    // we expext x: 22, z: 23
    printf("x: %d z: %d\n", x, z);
}
