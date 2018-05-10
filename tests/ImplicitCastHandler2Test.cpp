#include <cstdio>

int main()
{
unsigned int a = 5;
unsigned int b = 4;

printf("a %% b: %d %% %d = %d\n", a, b, (a%b));
  
signed int c = -5;
signed int d = 4;

printf("c %% d: %d %% %d = %d\n", c, d, (c%d));

signed int e = 5;
signed int f = -4;

printf("e %% f: %d %% %d = %d\n", e, f, (e%f));

signed char g = -5;
unsigned char h = 4;

printf("g %% h: %d %% %d = %d\n", g, h, (g%h));
  

signed int i = -5;
unsigned int j = 4;

printf("i %% j: %d %% %d = %d\n", i, j, (i%j));
printf("i %% j: %#x %% %#x = %#x\n", i, j, (i%j));    
}
