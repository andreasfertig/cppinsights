#include <tuple>

struct Point {
  	int data[2];
};

template <std::size_t N>
constexpr int& get(Point& p) {
  return p.data[N];
}

template <typename T> struct type_t { using type = T; };

namespace std {
  template <> struct tuple_size<Point> : integral_constant<size_t, 2> { };
  template <size_t N> struct tuple_element<N, Point> : type_t<int> { };
}

int f(Point p) {
  auto& [x, y] = p;
  return x;
}
