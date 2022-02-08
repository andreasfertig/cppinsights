#include <tuple>

auto tup = std::tuple<int, int>{2, 5};
auto [a1, b1] = tup;


int i = 5;
auto tup2 = std::tuple<int, int&>{2, i};
auto [a2, b2] = tup2;

