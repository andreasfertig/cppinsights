#include <cstdio>

int main()
{
  []() { printf("Hello "); }(), []() { printf("world"); }();
}
