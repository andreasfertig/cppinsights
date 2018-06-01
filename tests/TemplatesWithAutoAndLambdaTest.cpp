#include <iostream>

// Example taken from Nicolai Josuttis slides
template<auto sep = ' ',
         typename T1, typename... Types>
void print(const T1& arg1, const Types&... args)
{
    std::cout << arg1;
    auto coutSpaceAndArg = [](const auto& arg) {
      std::cout << sep << arg;
      };

   // we end up here with multiple parameters named 'args'. Other than that it would compile    
   (..., coutSpaceAndArg(args));     
}

int main()
{
  std::string str = "world";
  print( "hi", 7.5, str);
}
