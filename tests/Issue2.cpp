#include <utility>

struct Movable
{
  Movable() {}
  Movable(Movable&& other) {}
  Movable& operator=(Movable&& other)
  {
      return *this;
  }
  
  Movable(const Movable&) = delete;
  Movable& operator=(const Movable&) = delete;
};

int main()
{
    Movable m;
    int e;
    auto fun = [x = std::move(m), c=e] () -> void {
    };
    fun();

    auto bun = [c= std::move(e)] () -> void {
    };
    bun();
    
}
