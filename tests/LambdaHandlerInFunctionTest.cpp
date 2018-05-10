#include <cstdio>

template<typename T>
void Test(T&& t)
{
}

int main()
{
    Test([]{printf("Hello");});
    
    Test([]{printf("Hello"); return 1;}());
}
