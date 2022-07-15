// cmdline:-std=c++20

void NoParams()
{
    auto l = []() {
        static int n{};
        return ++n;
    };

    bool res = (1 == l());     // 1
    bool res2 = (2 == (+l)());  // 2
}

auto lWithParams = [](int x, int y) {
    static int n{};
    return ++n + x + y;
};

void WithParams()
{
    bool res = (6 == lWithParams(2, 3));     // 1
    bool res2 = (7 == (+lWithParams)(2, 3));  // 2
}

auto glambda = [](auto a) { return a; };

void WithParamsGeneric()
{
    int (*fp)(int) = glambda;
}

auto lWithParamsAndNTTP = []<int X = 3, int Z = 4>(int x, int y)
{
    static int n{};
    return ++n + x + y + X + Z;
};

void WithParamsAndNTTP()
{
    int res = lWithParamsAndNTTP.operator()<1, 0>(2, 3);
    int (*fp)(int, int) = lWithParamsAndNTTP;
}

int main()
{
    NoParams();
    WithParams();
}

