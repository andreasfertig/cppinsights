// cmdlineinsights:-edu-show-lifetime

struct Data
{
    int mData[5]{};
};

int main()
{
    const auto& v = Data{2, 3, 4};

    int dummy = 4;
}
