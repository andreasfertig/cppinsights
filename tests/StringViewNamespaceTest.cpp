#include <iostream>
#include <string>
#include <string_view>

int main(){
    
  std::string str = "Some string";
  std::string_view strView = str;
                 
  std::cout << "strView  : " << strView << std::endl;
}

