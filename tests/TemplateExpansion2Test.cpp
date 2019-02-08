#define INSIGHTS_USE_TEMPLATE

#include <cstddef>
#include <type_traits>

namespace details {
  template<class T>
  struct remove_reference      { typedef T type; };
  template<class T>
  struct remove_reference<T&>  { typedef T type; };
  template<class T>
  struct remove_reference<T&&> { typedef T type; };

  template<class T, size_t N = 0>
  struct extent
  {
    static constexpr size_t value = N;

    static_assert(N != 0, "Arrays only");
  };

  template<class T, size_t I>
  struct extent<T[I], 0>
  {
    static constexpr size_t value = I;

    static_assert(I != 0, "Arrays only");
  };

}  // namespace details

#define ARRAY_SIZE(var_x)                             \
  details::extent<typename details::remove_reference< \
    decltype(var_x)>::type>::value

class B
{
};

class E{};

template<typename T>
class S : public B
{
};

template<typename T>
class Y : public S<T>, public E
{
};


void test()
{
  int buffer[16]{};

  const auto& xx = ARRAY_SIZE(buffer);

  S<int> s; 

  Y<double> y;
}


int main(){}

