#include <tuple>

int main()
{
    std::tuple<int, int> tup{2,5};
    auto [a, b] = tup;
}
