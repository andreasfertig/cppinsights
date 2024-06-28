#include <tuple>

template<typename... C>
void
test(C...c)
{
  std::tuple<C...> tpl(c...);
}

int main()
{
  test(3, 5.5, 'a');
}
