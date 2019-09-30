class CTest
{
    auto Test() { return 22; }
};

auto Test()
{
    return 1;
}

auto Best() -> int
{
    return 1;
}

constexpr auto CEBest() -> int
{
    return 1;
}

decltype(auto) West()
{
    return 'c';
}

constexpr decltype(auto) CEWest()
{
    return 'c';
}

[[maybe_unused]] inline constexpr decltype(auto) MUCEWest()
{
    return 'c';
}

int main()
{
    int            x = 2;
    const char*    p;
    constexpr auto cei       = 0;
    auto constexpr cei2      = 0;
    auto                 i   = 0;
    decltype(auto)       xX  = (i);
    auto                 ii  = &i;
    auto&                ir  = i;
    auto*                ip  = &i;
    const auto*          cip = &i;
    auto*                pp  = p;
    const auto*          cp  = p;
    volatile const auto* vcp = p;
    auto                 f   = 1.0f;
    auto                 c   = 'c';
    auto                 u   = 0u;
    decltype(u)          uu  = u;

    [[maybe_unused]] auto        mu  = 0u;
    [[maybe_unused]] decltype(u) muu = u;
}
