#include <cstdio>

class A
{
public:
    int          a;
    virtual void v() { puts("A->v"); }
};

class B
{
public:
    int          b;
    virtual void w() { puts("B->w"); }
};

class C : public A, public B
{
public:
    int  c;
    void w() { puts("C->w"); }
};

class D : public A, public B
{
public:
    int d;
};

class Apple
{
public:
    Apple() {}

    Apple(int x)
    : mX{x}
    {
    }

    ~Apple() { mX = 5; }

    Apple(const Apple&) {}

    void Set(int x) { mX = x; }
    int  Get() const { return mX; }

private:
    int mX{};
};

void MemberFunctions()
{
    Apple a{};

    a.Set(4);

    Apple* paaa{};
    paaa->Set(5);

    Apple b{6};
    Apple c{b};

    b = a;
}

void Inheritance()
{
    C c;

    c.w();
    c.v();

    B* b = &c;
    b->w();

    C* cb = (C*)(b);
    cb->v();

    //
    D  d;
    B* bd = &d;
    D* db = (D*)bd;
    db->w();
}

int main()
{
    MemberFunctions();

    Inheritance();
}
