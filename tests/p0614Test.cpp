// cmdline:-std=c++20

#include <vector>

struct T {
    std::vector<int> _items{};

    auto& items() const { return _items; }
};

T f() { return T{}; }

int main() {
    for (T thing = f(); auto& x : thing.items()) {
    }
}
