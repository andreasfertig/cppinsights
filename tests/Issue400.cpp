// cmdline:-std=c++20

#include <concepts>

auto foo(auto x)
{
   if constexpr (std::same_as<int, decltype(x)>)
   {
      return 1;
   }
   return 2;
}

int main()
{
	foo(1);
    foo(1.0);
}
