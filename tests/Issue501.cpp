// cmdlineinsights:-show-all-callexpr-template-parameters

#include <utility>
static int s1;
static int s2;
std::pair<int const&, int const&> fun() { return std::make_pair(s1, s2);
}
