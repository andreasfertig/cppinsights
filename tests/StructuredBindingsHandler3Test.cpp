//#include <tuple>
using size_t = decltype(sizeof(0));
// Without this the compiler does not know a symbol tuple_size and hence can/will not decompose
namespace std { template<typename T> struct tuple_size; }
namespace std { template<size_t, typename> struct tuple_element; } // expected-note 2{{here}}


namespace constant {
  struct Q {};
  template<int N> constexpr int get(Q &&) { return N * N; }
}
template<> struct std::tuple_size<constant::Q> { static const int value = 3; };
template<size_t N> struct std::tuple_element<N, constant::Q> { typedef int type; };
namespace constant {
  Q q;
  // This creates and lifetime-extends a temporary to hold the result of each get() call.
  auto [a, b, c] = q;    // expected-note {{temporary}}
  constexpr bool f() {
    auto [a, b, c] = q;
    return a == 0 && b == 1 && c == 4;
  }

  constexpr int g() {
    int *p = nullptr;
    {
      auto [a, b, c] = q;
      p = &c;
    }
    return *p; // expected-note {{read of object outside its lifetime}}
  }
}
