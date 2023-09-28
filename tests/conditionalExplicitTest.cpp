// cmdline:-std=c++20

#include <type_traits>

struct B {};
struct C {};

struct A {
  A() = default;
  explicit(true) A(const B&) {}
  explicit(false) A(const C&) {}
  explicit(true) operator B() const { return {}; };
};

template<typename T>
struct Wrapper {
  template<typename U>
  explicit(not std::is_convertible_v<U, T>) Wrapper(const U&) {}
};

void Fun(Wrapper<A> a);  // #A Takes Wrapper<A> now

void Use()
{
  Fun(A{});
  Fun(static_cast<A>(B{}));  // #B Does compile!
  Fun(C{});  // #B Does compile!
}

void Fun(Wrapper<A> a) {}

int main()
{
  Use();
}
