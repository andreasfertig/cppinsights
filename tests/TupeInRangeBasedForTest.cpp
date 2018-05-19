#include <cstdio>
#include <string>
#include <tuple>
#include <vector>

template <typename Tuple_t>
void fillTuples(std::vector<Tuple_t> tuples) {
  tuples.emplace_back("hello", 1);
}

int main() {
  std::vector<std::tuple<std::string, int>> tuples;
  fillTuples(tuples);
    
  for (auto [s, n] : tuples) {
    printf("c=%s, n=%d\n", s.c_str(), n);
  }
}

