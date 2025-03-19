// cmdline:-std=c++20
 
struct S
{
    int x;
};

void foo()
{
    S s{1};
    auto& [x] = s;
    auto g    = [x]() {};
}

void bar()
{
    int arr[]{1, 2};
    auto& [x, _] = arr;
    auto g       = [x]() {};
}
