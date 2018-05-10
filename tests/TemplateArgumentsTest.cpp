#include <cstdio>
#include <string>
template<typename T>
static inline decltype(auto) Normalize(const T& arg)
{
  if constexpr(std::is_same_v<T, std::string>) {
    return arg.c_str();
  } else {
    return arg;
  }
}

template<typename... Ts>
void Log(const char* fmt, const Ts&... ts)
{
  printf(fmt, Normalize(ts)...);
}

int main()
{
  int result = 0x22;

  Log("Hello World %s\n", "nothing");

  Log("And the number is %d\n", result);

  std::string test{"Hello"};
  Log("%s\n", test);
}
