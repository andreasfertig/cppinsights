#include <tuple>

std::tuple<int, float> foo;
 
auto [a, b] = foo;
auto& [ra, rb] = foo;


char c[3]{};

auto [x, y, z] = c;

auto& [cx, cy, cz] = c;


std::tuple<int, float> Func();
 
const auto& [fa, fb] = Func();
