struct Data
{
    int mData[5]{};

    const int* begin() const { return mData; }
    const int* end() const { return mData + 1; }
};

struct Keeper
{
    Data data{2, 3, 4};

    auto& items() const { return data; }
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
