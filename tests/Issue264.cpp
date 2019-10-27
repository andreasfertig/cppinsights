// Move it outside of main to get compiling code. As user is not allowed to declare a template inside a local class.
auto f = [](auto... i) { return (.../i); };
auto g = [](auto... i) { return (i/...); };

int main()
{
  f(1,2,3);
  g(1,2,3);
}
