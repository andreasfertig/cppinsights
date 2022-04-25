// cmdlineinsights:-edu-show-cfront

#include <cstdio>

struct A
{
};

struct B : A
{
};

struct C
{
    C(int) {}
};

struct D
{
    int x;

    D() {}
};

struct E
{
    int x;

    ~E() {}
};

int main()
{
    A avec[4];

    A* a = new A[2];

    // C cvec[3]{2,3,4};

    C* c = new C[2]{{2}, {3}};

    C* c2 = new C{3};

    D dvec[5];

    E e;

    E* ep = new E;

    delete[] a;
    delete[] c;
    delete c2;
    delete ep;
}
