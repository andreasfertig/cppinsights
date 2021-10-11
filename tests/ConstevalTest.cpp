// cmdline:-std=c++20

consteval bool ConstantFun()
{
    return true;
}

static_assert(ConstantFun());

struct Test {
    consteval bool Fun() { return true; }
};

static_assert(Test{}.Fun());

static_assert([]() consteval { return true; }());

