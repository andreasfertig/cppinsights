#include <chrono>

using seconds_t      = std::chrono::seconds;

constexpr seconds_t operator ""_s(unsigned long long s)
{
    return seconds_t(s);
}

int main()
{
    auto s = 1_s;
}
