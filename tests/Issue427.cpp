#include <iostream>

template<int val>
class MyClass
{
public:
    static int var;
};

template<int val> int MyClass<val>::var = val;

int main(int argc, char* argv[])
{
    MyClass<5> a;
    MyClass<7> b;
    MyClass<9> c;

    std::cout << a.var <<  " , " << b.var <<  " , " << c.var << std::endl;

    return 0;
}

