// cmdlineinsights:-edu-show-initlist
#include <initializer_list>

int main(int argc, const char*[])
{
    int i;
    int x;
    auto list = std::initializer_list<int>{i, x};

    auto lamb = [=]{ return list; };

    auto l = lamb();

    auto lamb2 = [=]{ return std::initializer_list<int>{i, x}; };

    auto l2 = lamb2();
}


