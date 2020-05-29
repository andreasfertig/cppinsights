// cmdline:-std=c++2a
#include <compare>

struct Spaceship
{
    int x;

    constexpr auto operator<=>(const Spaceship& value) const = default;
};

int main() {
    constexpr Spaceship enterprise{2};

    constexpr Spaceship millenniumFalcon{2};

    // This will be rewritten: std::operator>=(s.operator<=>(y), 0)
    static_assert(enterprise <= millenniumFalcon);

    static_assert(enterprise == millenniumFalcon);
}

