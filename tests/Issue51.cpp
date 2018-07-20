#include <map>

int main()
// no open curly bracket here
    using Map = std::map<int, bool>;
    Map m;
    auto [it, ok] = m.insert({1, true});
}
