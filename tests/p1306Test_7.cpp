// cmdline:-std=c++26

#include <cstdio>


int main()
{
  int res{};
  template for(auto x : {1, 2, 3})
  {
    res += x;

    if(x == 2) { continue; }

    if(x == 3) { break; }

    printf("%d\n", x);
  }

  printf("end\n");
}
