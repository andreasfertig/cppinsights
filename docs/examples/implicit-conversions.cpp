#include <cstdio>
template<typename U>
class X
{
public:
    X()           = default;
    X(const X& x) = default;

    template<typename T>
    X(T&& x)
    : mX{}
    {
    }

private:
    U mX;
};

int main()
{
    X<int> arr[2]{};

    // We use X<const int> instead of X<int> here. This results
    // in a constructor call to create a X<const int> object as
    // you can see in the transformation.
    for(const X<const int>& x : arr) {
    }
}
