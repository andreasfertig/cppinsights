// cmdline:-std=c++26

#define INSIGHTS_USE_TEMPLATE 1

#include <tuple>

int main()
{
  std::tuple<int, char, long> tpl{1, 'a', 3l};
  int                         arr[]{4, 5, 6};
  int                         res{};

  template for(auto& e : tpl)
  {
    res += static_cast<int>(e);
  }

  template for(const auto& e : arr)
  {
    res += e;
  }

  template for(auto x : {1, 2, 3})
  {
    template for(auto y : {10, 20}) { res += x * y; }
  }

  return res;
}
