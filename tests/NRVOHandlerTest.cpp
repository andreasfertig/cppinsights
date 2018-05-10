#include <cstdio>

struct C
{
    C() = default;
    C(const C&) { printf("Copy\n"); }
};

C f()
{
    C namedC{};
    
    return namedC;
}

C f2()
{
    C namedC{};
    C namedC2{namedC};
    
    return namedC2;
}


int main()
{
    printf( "Hello\n");

    C obj = f();    
    C obj2 = f2();    
}
