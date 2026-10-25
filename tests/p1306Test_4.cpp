// cmdline:-std=c++26

#include <string_view>

int main()
{
  static constexpr std::string_view foo = "abcd";
  template for(auto x : foo)
  {
    // ...
  }
}
