#include <iostream>

struct A
{
    A() = default;

    A& begin()
    {
       return *this;
    }

    const int end()
    {
      return v[9];
    }

    A& operator++()
    {
        return *this;
    }

    int operator*() { return 1; }

    int v[10]{};
};

bool operator!=(const A&, const int&){ return true; }



int main()
{
    A a;
    for( const auto& it : a )
    {
        std::cout << it << std::endl;
    }
}

