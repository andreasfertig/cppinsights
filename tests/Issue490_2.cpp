// cmdline:-std=c++20

using x = decltype([](){ return 4; });
auto foo() -> x;
