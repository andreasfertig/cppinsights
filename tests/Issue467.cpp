#include <iostream>

int main()
{
    auto l = []()
    {
      static int n{};
      std::cout << ++n;
    };

  l();
  l();
  l();
  l();
  (+l)();
  (+l)();  
}
