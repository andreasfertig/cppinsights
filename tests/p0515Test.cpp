// cmdline:-std=c++2a
#include <compare>

struct Spaceship
{
    int x;

    constexpr auto operator<=>(const int& value) const
    {
        if(x == value) {
            return std::strong_ordering::equal;
        } else if(value > x) {
            return std::strong_ordering::less;
        } else {
            return std::strong_ordering::greater;
        }
    }

    constexpr bool operator==(const int& value) const
    {
        return *this <=> value == 0;
    }
};

int main() {
    constexpr int y = 2;

    constexpr Spaceship millenniumFalcon{3};

    // This will be rewritten: std::operator>=(s.operator<=>(y), 0)
    static_assert(millenniumFalcon >= y);
    
    static_assert(y <= millenniumFalcon);

    static_assert(not (y == millenniumFalcon));
    static_assert(not (millenniumFalcon == y));
}

