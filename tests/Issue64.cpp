#include <string>

void func(const std::string& arg) {
 
  auto s =[arg] {
    return arg.size();
  }();
}

int main() {
  std::string b;
  
  func(b);
}
