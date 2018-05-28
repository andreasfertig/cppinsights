#include <map>

int main()
{
    std::map<int, int> map{{1, 2}};
    auto [key, value] = *map.begin();
}
