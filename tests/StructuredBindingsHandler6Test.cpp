#include <cstdio>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>

using namespace std;

class Point;
template<>
struct std::tuple_size<Point> : std::integral_constant<size_t, 2>
{
};
template<>
struct std::tuple_element<0, Point>
{
    using type = double;
};
template<>
struct std::tuple_element<1, Point>
{
    using type = double;
};

class Point
{
public:
    constexpr Point(double x, double y) noexcept
    : mX(x)
    , mY(y)
    {
    }

    constexpr double GetX() const noexcept { return mX; }
    constexpr double GetY() const noexcept { return mY; }

    constexpr void SetX(double x) noexcept { mX = x; }
    constexpr void SetY(double y) noexcept { mY = y; }

private:
    double mX, mY;

public:
    template<size_t N>
    constexpr decltype(auto) get() const noexcept
    {
        if constexpr(N == 1) {
            return GetX();
        } else if constexpr(N == 0) {
            return mY;
        }
    }
};

int main()
{
#if 1
    Point p     = Point{1, 2};
    auto [x, y] = p;

    printf("x:%lf y:%lf\n", p.GetX(), p.GetY());
    printf("x:%lf y:%lf\n", x, y);
#endif

    char ar[2];
    auto [a1, a2]  = ar;
    auto& [a4, a5] = ar;

    const auto& [a8, a9] = ar;

    const char aa[2]       = {1, 2};
    const auto& [a18, a19] = aa;

    auto muple = std::make_tuple(1, 'a', 2.3);

    // unpack the tuple into its individual components
    auto& [i, c, d] = muple;

    auto [ii, cc, dd] = muple;

#if 0
    struct SP
    {
        int x;
        int y;
    };

    SP sp{1, 2};

    auto[sx, sy] = sp;
#endif

#if 0
    class CP
    {
    public:
        int x;
        int y;
    };

    CP cp{1,2};

    auto [ c1, c2 ] = cp;
#endif
}
