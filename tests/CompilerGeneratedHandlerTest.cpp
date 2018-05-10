#include <utility>

struct S
{
    int i;
};

struct SS
{
    int i;
};


class C
{
public:
    int i;
};

class D
{
public:
    int i;
};

class E
{
public:
    
    E(int x): i{x} {}
private:
    int i;
};

int main()
{
    S s{1};

    S s2 = s;
    S s3 = std::move(s);

    C c{2};

    C c2 = c;
    C c3 = std::move(c);


    D d{2};

    D d2 = d;
    D d3 = std::move(d);
    D d4;
    d4 = d2;

    SS ss{2};

    SS ss2 = ss;
    SS ss3 = std::move(ss);
    SS ss4;
    ss4 = ss2;

    D* dd = new D{3};

    delete dd;

    E e{6};

}
