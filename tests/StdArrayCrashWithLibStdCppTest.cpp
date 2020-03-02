#include <array>

int foo()
{
    std::array<int, 20> localArray{0};
    return localArray[1];
}

int main()
{
   return foo();
}

