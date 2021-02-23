#include <algorithm>

struct C {
  int f() {
    int a = 5;
  	const auto b = [&]{return m+a;};
  	return b();
  }
  int m = 6;
};
