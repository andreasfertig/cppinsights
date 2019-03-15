// cmdline:-std=c++11
#include <cstdio>
#include <vector>


int main()
{
    char arr[10]{2,4,6,8,10,12,14,16,18,20};

    for(char c : arr)
        ;

    std::vector<int> v{1, 2, 3, 5};

    for(auto& vv : v)
        ;

    for(auto& vv : v);   
}
