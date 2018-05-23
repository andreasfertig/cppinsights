#include <iostream>
#include <memory>

std::shared_ptr<int> create() {
    return std::make_shared<int>(42);
}

int main()
{
  const auto& car = create(); 
  const auto&& cart = create();
  auto&& carte = create();

  const auto& vcar = create(); 
  const volatile auto&& cvcart = create();
  volatile auto&& vcart = create();
  volatile auto&& vcarte = create();
}
