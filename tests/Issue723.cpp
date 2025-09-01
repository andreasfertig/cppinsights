#include <map>
#include <string>

int main() {
    std::map<int, std::string> m;
    for (const auto& [k, v] : m) {
      decltype(k) a = 1;
    }
}
