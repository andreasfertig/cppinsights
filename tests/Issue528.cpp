// cmdlineinsights:-alt-syntax-for
#include <initializer_list>

int main() {
  for (int i : {0,1}) {
    if (i == 0) continue;

    if (i == 5) continue;
  }
}
