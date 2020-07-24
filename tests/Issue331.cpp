#define foo(x) do { return (x); } while (0)

int f(int num)
{
  if (num > 0)
    foo(num);
  else
    return num * num;
}
