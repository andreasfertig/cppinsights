// cmdline:-std=c++2a

#include <cstdio>
#include <type_traits>

namespace p0892 {
    struct simple {
        explicit(false) simple(int) {}
    };

template<class T>
struct wrapper {

    template<class U>
    explicit(not std::is_convertible_v<U, T>)
    wrapper(const U& u)
    {}

    template<class U>
    explicit(not std::is_convertible_v<T, U>)
    operator U() const
    {
        return {};
    }
};

struct B {};

struct A {
    A() = default;
    explicit A(const B&) {}
    explicit operator B() const { return {}; }
};

void Fun(wrapper<A> a)
{
    B b = static_cast<B>(a);
}
}

int main()
{
    using namespace p0892;
    
    Fun(A{});
    Fun(static_cast<A>(B{}));
}

