#include <chrono>
#include <string>

std::chrono::seconds operator"" _s(unsigned long long s) {
    return std::chrono::seconds(s);
}

std::string operator"" _str(const char *s, std::size_t len) {
    return std::string(s, len);
}


template<typename T, T... C>
constexpr int operator""_x()
{
  return 0;
}

int main()
{
    using namespace std::literals;
    std::chrono::seconds t = 98291919s;
    
    auto t4 = "12345"_x;

    auto str = "abcd"_str;
    auto sec = 4_s;
}
