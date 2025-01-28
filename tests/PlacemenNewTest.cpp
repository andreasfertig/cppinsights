#include <new>

template<typename T>
struct Apple
{
    T x;
};


int main()
{
    char buffer[sizeof(Apple<int>)];

    auto* f = new (&buffer) Apple<int>;

    auto& r = *f;

    r.~Apple<int>();

    f->~Apple<int>();
}
