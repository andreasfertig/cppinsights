#include <cstdio>
#include <type_traits>
#include <utility>

namespace details::array_single_compare {
  template<typename T, size_t N, typename TTo, size_t... I>
  constexpr bool Compare(const T (&ar)[N], const TTo& to, std::index_sequence<I...>)
  {
    return ((to == ar[I]) && ...);
  }
} /* namespace details::array_single_compare */

template<typename T, size_t N, typename TTo>
constexpr bool Compare(const T (&ar)[N], const TTo& to)
{
  return details::array_single_compare::Compare(ar, to, std::make_index_sequence<N>{});
}

namespace details::array_compare {
  template<typename T, size_t N, size_t... I>
  constexpr bool Compare(const T (&ar)[N], const T (&to)[N], std::index_sequence<I...>)
  {
    return ((to[I] == ar[I]) && ...);
  }
} /* namespace details::array_compare */

template<typename T, size_t N>
constexpr bool Compare(const T (&ar)[N], const T (&to)[N])
{
  return details::array_compare::Compare(ar, to, std::make_index_sequence<N>{});
}


struct MACAddress
{
  unsigned char value[6];
};

void Main()
{
  const MACAddress macA{1, 1, 1, 1, 1, 1};
  const MACAddress macB{1, 1, 1, 1, 1, 2};
  const MACAddress macC{1, 1, 1, 1, 1, 1};

  printf("Equal: %d\n", Compare(macA.value, 1));
  printf("Not equal: %d\n", !Compare(macB.value, 1));
  printf("---------------\n");
  printf("Not equal: %d\n",
         !Compare(macA.value, macB.value));
  printf("Equal: %d\n",
         Compare(macA.value, macC.value));
}

