// cmdline:-std=c++23

#include <vector>

int main() {
  std::vector<int> v{0, 1, 2, 3};
  for (auto i = 0uz, s = v.size(); i < s; ++i) {
	/* use both i and v[i] */
  }
}
