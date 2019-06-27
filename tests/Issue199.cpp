template<typename U, typename ...T>
void f(U, T... rest)
{
  if constexpr (sizeof...(rest) != 0)
    f(rest...);
}

int main()
{
    f(0, 1);
}
