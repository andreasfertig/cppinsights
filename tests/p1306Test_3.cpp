// cmdline:-std=c++26

#define INSIGHTS_USE_TEMPLATE 1

#include <tuple>

template<typename T>
void Call(const T&)
{}

template<typename T>
void Fun()
{
  std::tuple<T, long, unsigned> tpl{1, 2l, 3u};
  template for(int idx{}; auto x : tpl)
  {
    Call(x);
  }
}

int main()
{
  Fun<int>();
}
