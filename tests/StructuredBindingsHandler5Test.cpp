#include <cstdio>
#include <type_traits>
#include <utility>

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

    template<size_t N>
    constexpr decltype(auto) get() noexcept
    {
        if constexpr(N == 1) {
            return GetX();
        } else if constexpr(N == 0) {
            return (mY);
        }
    }

    template<size_t N>
    constexpr decltype(auto) get() const noexcept
    {
        if constexpr(N == 1) {
            return GetX();
        } else if constexpr(N == 0) {
            return (mY);
        }
    }

private:
    double mX, mY;
};

int main()
{
    Point p      = Point{1, 2};
    auto& [x, y] = p;

    printf("x:%lf y:%lf\n", p.GetX(), p.GetY());
    x++;
    printf("x:%lf y:%lf\n", x, y);
    printf("x:%lf y:%lf\n", p.GetX(), p.GetY());

    constexpr Point p2 = Point{3, 4};
    auto& [x2, y2]     = p2;

    // error: decomposition declaration cannot be declared 'constexpr'
    // constexpr auto [x3, y3] = p2;
}
