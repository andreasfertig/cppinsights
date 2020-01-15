#include <vector>
struct S {
	std::vector<int> v{};
    decltype(v)& getV () {
      return v;
    }
};

