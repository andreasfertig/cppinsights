// cmdline:-std=c++26

template<typename... T>
constexpr auto first_plus_last(T... values) -> T...[0]
{
  return T...[0](values...[0] + values...[sizeof...(values) - 1]);
}

// first_plus_last(); // ill formed
static_assert(first_plus_last(1, 2, 10) == 11);

auto res = [](auto... pack) {
  decltype(pack...[0])   x5;      // type is int
  decltype((pack...[0])) x6{x5};  // type is int&

  return 0;
}(0, 3.14, 'c');

int main() {}

