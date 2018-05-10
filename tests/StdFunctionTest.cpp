#include <functional>
#include <cstdio>

void Func() {}

template<typename T>
void Test(T& t) 
{
}

int main()
{
    std::function<void()> x = Func;

  Test(x);
}
