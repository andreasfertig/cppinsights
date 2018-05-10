#include <map>
#include <set>

template <typename T> void foo(T & t)
{
    auto it = t.find(42);
}


int main() {
std::map<int, int> m;
std::set<int> s;

foo(m);
foo(s);
}
