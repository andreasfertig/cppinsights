template <typename T>
struct FunctionArgs  {};

template <typename R, typename C, typename... Args>
struct FunctionArgs<R(C::*)(Args...) const>  {};


int main()
{
    int y = 2;
    int z = 3;
    auto l = [&](int x) { return x + y; };

    FunctionArgs<decltype(&decltype(l)::operator())> a;
}
