// cmdline:-std=c++26

#define INSIGHTS_USE_TEMPLATE 1

template<typename T,
         typename... Args  // #A Variadic template
         >
constexpr T min(const T& x,
                const T& y,
                const Args&... vals)  // #B Parameter pack
{
  auto m = x < y ? x : y;

  template for(int init{3}; const auto& e : {vals...})
  {
    if(e < m) { m = e; }
  }

  return m;
}

static_assert(min(3, 2) == 2);
static_assert(min(3, 2, 3, 4, 5) == 2);

int main() {}
