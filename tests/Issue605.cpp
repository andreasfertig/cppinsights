// cmdline:-std=c++23

#include <cstdio>
#include <functional>
#include <memory>

using namespace std;

int main()
{
    [a=make_unique<int>(42)]() {};
}
