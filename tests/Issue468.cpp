// cmdline:-std=c++20

namespace A{
template<class F = decltype([]() { return true; })>
bool test(F f = {})
{
  return f();
}
}

int main()
{
    A::test();
}
