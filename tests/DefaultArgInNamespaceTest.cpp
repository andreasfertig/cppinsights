#include <iostream>

namespace Test {
enum class Order {
    A,
    B,
};

inline constexpr Order OrderA = Order::A;

class West
{
    public: West() {}

    int load(const Order order = OrderA) { return 2; }
};
}

int main()
{
    Test::West w;

    std::cout << "test: " << w.load() << "\n";
}
