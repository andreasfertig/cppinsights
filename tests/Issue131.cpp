#include <tuple>

std::tuple<int, float> foo();
 
auto [a, b] = foo();
