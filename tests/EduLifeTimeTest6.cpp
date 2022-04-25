// cmdlineinsights:-edu-show-lifetime

#include <vector>

struct Keeper
{
    std::vector<int> data;

    auto items() const { return data; }
};

Keeper get()
{
    return {};
}

int main()
{
    for(auto& item : get().items()) {
    }
}
