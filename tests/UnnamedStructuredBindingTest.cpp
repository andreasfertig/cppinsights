using X = void (*) ();

struct RET
{
    X x;
    int r;
};

int main()
{
    const auto& [t, e] = RET{};
}

