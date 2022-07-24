// cmdline:-std=c++20

template<class F = decltype([]() { return true; })>
bool test(F f = {})
{
  return f();
}


int main()
{
    test();
}
