// Source: https://en.cppreference.com/w/cpp/language/structured_binding
#include <tuple>

float x{};
char  y{};
int   z{};
 
std::tuple<float&,char&&,int> tpl(x,std::move(y),z);
const auto& [a,b,c] = tpl;
// a names a structured binding that refers to x; decltype(a) is float&
// b names a structured binding that refers to y; decltype(b) is char&&
// c names a structured binding that refers to the 3rd element of tpl; decltype(c) is const int
