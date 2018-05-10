#include <iostream>

enum class Color{
  red,
  blue,
  green
};
 
enum struct State: char {
  on,
  off,
};

int main(){

  // UnaryExprOrTypeTraitExpr with Type
  std::cout << sizeof(Color) << std::endl;  
  std::cout << sizeof(State) << std::endl;

}

