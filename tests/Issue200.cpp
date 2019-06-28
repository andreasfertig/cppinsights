template<typename U, typename ...T>
void f(U, T... rest)
{
  if constexpr (sizeof...(rest) != 0)
    f(rest...);
}

template<>
void f<int>(int) {}

int main()
{
    f(0, 1);
}
