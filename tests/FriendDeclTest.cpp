#include <stdio.h>

class Foo
{
public:
    Foo()
    : mI{0}
    {
    }

    void Print() { printf("%d\n", mI); }

private:
    class Secrect {};
    friend class DearFriend;

    int mI;

    friend void ChangePrivate(Foo&);
};

void ChangePrivate(Foo& i)
{
    ++i.mI;
}

class DearFriend : public Foo::Secrect
{};


int main()
{
    Foo sPoint;
    sPoint.Print();
    ChangePrivate(sPoint);
    sPoint.Print();
}
