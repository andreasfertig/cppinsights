#include <cstdio>
#include <algorithm>
#include <vector>

struct A
{
    A()
    {
        int i = 0;
        std::generate(&v[0], &v[10], [&i]() { return ++i; });
    }

    int v[10];
};


struct B
{
    B()
    {
        int i = 0;
        std::generate(&v[0], &v[10], [&i]() { return ++i; });
    }

    
    bool operator!=(const B& rhs){
        return this != &rhs;
    }


    int* begin()
    {
        return &v[0];
    }

    int* end()
    {
        return &v[10];
    }


    int v[10];
};

int* begin(A& v)
{
    return &v.v[0];
}

int* end(A& v)
{
    return &v.v[10];
}

int main()
{
    char arr[10]{2,4,6,8,10,12,14,16,18,20};

    for(char c : arr) {
        printf("%#x\n", c);
        c++;
        printf("%#x\n", c);
    }

    std::vector<int> v{1, 2, 3, 5};

    for(auto& vv : v) {
        printf("%d\n", vv);
    }

    for(auto& v2 : v)
        printf("%d\n", v2);

    A a;
    for(auto it : a) {
        printf("%d\n", it);
    }

    B b;
    for(auto bit : b) {
        printf("%d\n", bit);
    }
    
}
