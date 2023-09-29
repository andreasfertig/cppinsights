// cmdline:-std=c++20

#include <queue>
#include <vector>

void f()
{
    std::priority_queue<int,
                        std::vector<int>,
                        decltype([](int lhs, int rhs) { return lhs > rhs; })> min_heap;
}
