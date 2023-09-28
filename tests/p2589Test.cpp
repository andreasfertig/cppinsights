// cmdline:-std=c++2b

struct Test
{
    static auto operator[](unsigned idx) {
        return 2;
    }
};

int main()
{
    Test t{};

    return t[4];
}
