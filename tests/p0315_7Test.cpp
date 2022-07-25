// cmdline:-std=c++20

template<class F = decltype([]() -> bool { return true; })>
bool test = true; // fails without initializer

//template<class F = decltype([]() -> bool { return true; })>
//class X{};

int main()
{
    auto t = test<>;

//    X x{};
}

