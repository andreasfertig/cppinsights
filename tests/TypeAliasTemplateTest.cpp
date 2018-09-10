template<bool B>
struct bool_constant
{
  static const bool value = B;
};

template<typename... A>
struct F
{
  template<typename... B>
    using SameSize = bool_constant<sizeof...(A) == sizeof...(B)>;

  template<typename... B, typename = SameSize<B...>>
  F(B...) { }
};

int main()
{
  F<int> f1(3);
}
