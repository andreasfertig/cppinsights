namespace Test {
template<typename T>
struct Base
{
    static constexpr bool value{true};
};


template <typename T, typename = void>
struct has
{
  static constexpr bool value = false;
  using type = T;
};

template <typename T, bool = has<T>::value>
struct dependentScope
{
  static constexpr bool result = true;
};

template <typename T>
struct dependentScope2
{
  static constexpr bool result = has<T>::value;
};


template <typename T>
struct dependentScope3
{
  template<typename U>
  static constexpr bool result = has<T, U>::value;
};


template <typename T>
struct dependentScope4
{
  static constexpr typename has<T>::type  result = has<T>::value;
};

}

int main()
{
    auto a = Test::dependentScope<int>::result;
    auto b = Test::dependentScope2<int>::result;
    auto c = Test::dependentScope3<int>::result<char>;
    auto d = Test::dependentScope4<bool>::result;
}
