#include <cstdio>

int main()
{
  enum class Colour{red= 0,green= 2,blue};
 
  printf("%d %d %d\n", Colour::red, Colour::green, Colour::blue);
}
