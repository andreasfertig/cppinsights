#define INSIGHTS_USE_TEMPLATE

template<typename T>
constexpr T min(const T& a, const T& b)
{
    return (a < b) ? a : b;
} 

template<typename T>
static T max(const T& a, const T& b)
{
    return (a > b) ? a : b;
} 

int main()
{
    int a=1, b=2;

    int mi = min(a, b);

    double ad=2.4, bd=3.4;

    auto md = min( ad, bd);

    int ma1 = max(1, 2);
    double ma2 = max(2.0, 4.0);
}

