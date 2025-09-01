template <typename T>
struct FunctionArgs{};


template <typename R, typename... Args>
struct FunctionArgs<R(*)(Args...)>  {};



template <typename R, typename C, typename... Args>
struct FunctionArgs<R(C::*)(Args...) const>  {};


template<typename T>
struct myString
{
};

using string = myString<char>;

struct Test
{
    string Fun(double) const { return {}; }
};


FunctionArgs<decltype(&Test::Fun)> a;

