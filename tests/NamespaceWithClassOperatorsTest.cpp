#include <chrono>

std::chrono::duration<double> Bar(){
  auto begin= std::chrono::system_clock::now();
  
  return std::chrono::system_clock::now() - begin;
};


void Foo(){
  auto start = std::chrono::system_clock::now();

  std::chrono::duration<double> theDuration= std::chrono::system_clock::now() - start;
}


int main(){
    Bar();
    Foo();
}

