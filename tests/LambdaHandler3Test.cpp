template<typename T>
int Is(const T x)
{
  return x(22);
}

int main()
{
  int y = 3;
  return Is([&](int x){ return x +y; });
}

