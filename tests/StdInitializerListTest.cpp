// cmdlineinsights:-edu-show-initlist
#include <cstdio>
#include <initializer_list>

auto f(int i, int j, int k)
{
  return std::initializer_list<int>{i, j, k};
}

auto f1(int i)
{
  return std::initializer_list<int>{i};
}


int main(int argc, const char*[])
{
    for(int i : f(argc+1, argc+2, argc+3)) {
      printf("%d, ", i);
    }

    auto ff = f1(2);
}

