// cmdlineinsights:-edu-show-cfront

// cmdlineinsights:-edu-show-lifetime

struct list
{
    char data[5];

    char* begin() { return data; }
    char* end() { return &data[5]; }
};

struct Keeper
{
    list data{};

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
