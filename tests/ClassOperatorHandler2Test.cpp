#include <iostream>

void f(int&&)
{
    std::cout << "hello"
              << "&&\n";
}
void f(int&)
{
    std::cout << "bello"
              << "&\n"
<< "world" << "\n";
}

class Foo
{
public:
    Foo(int x)
    : mX{x}
    {
    }

    operator const int() const { return mX; }
    Foo operator++(int)
    {
        Foo old(*this);
        printf("  ======\n");
        ++mX;
        printf("  ++++++\n");
        return old;
    }
    Foo& operator++()
    {
        ++mX;
        return *this;
    }

    Foo operator--(int)
    {
        Foo old(*this);
        printf("  ======\n");
        --mX;
        printf("  ++++++\n");
        return old;
    }
    Foo& operator--()
    {
        --mX;
        return *this;
    }
    
    int Get() const { return mX; }

private:
    int mX;
};

int main()
{
    []() {
    auto n = 10;
    f(n);      // identifier of a variable is almost always an lvalue
    f(42);     // 42 is a prvalue (like most literals)
    f(int{});  // int{} is a prvalue

    Foo f(1);

    ++f;
    f++;

    --f;
    f--;

    printf("%d\n", f++);
    printf("%d %d\n", f++, 1);

    printf("%d\n", f--);
    printf("%d %d\n", f--, 1);
    
    Foo* ff = &f;

    printf("%d\n", ff->Get());

    ++ff;
    ff++;
    ++++ff;

    --ff;
    ff--;
    ----ff;
    
    printf("%d\n", f.Get());
    }();


    auto n = 10;
    f(n);      // identifier of a variable is almost always an lvalue
    f(42);     // 42 is a prvalue (like most literals)
    f(int{});  // int{} is a prvalue

    Foo f(1);

    ++f;
    f++;

    --f;
    f--;

    printf("%d\n", f++);
    printf("%d %d\n", f++, 1);

    printf("%d\n", f--);
    printf("%d %d\n", f--, 1);
    
    Foo* ff = &f;

    printf("%d\n", ff->Get());

    ++ff;
    ff++;
    ++++ff;

    --ff;
    ff--;
    ----ff;
    
    printf("%d\n", f.Get());

}
