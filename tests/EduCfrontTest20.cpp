// cmdlineinsights:-edu-show-cfront

#include <utility>

class X
{
public:
    X(std::initializer_list<int> x) {}
};

void fun()
{
    X{1, 2, 3};
}
