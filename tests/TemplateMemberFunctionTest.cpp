#include <cstdio>

class Foo
{
public:
    Foo() = default;

    template<typename T>
    void Do(T&& t)
    {
        t();
    }


    template<int X, typename T>
    void DoOther(T&& t)
    {
        t();        
    }

    template<int X, char c, typename T>
    void DoBar(T&& t)
    {
        t();        
    }
    
};

int main()
{
    Foo f;

    f.Do([]() { printf("hello\n"); });

    f.DoOther<1>([]() { printf("hello\n"); });

    f.DoBar<1, 't'>([]() { printf("hello\n"); });
}
