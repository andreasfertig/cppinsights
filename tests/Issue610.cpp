#include <iostream>
#include <limits.h>

int main (int argc, char *argv[]) {
  const int a = 10;
  int *p = (int *)&a;
  return 0;
}
