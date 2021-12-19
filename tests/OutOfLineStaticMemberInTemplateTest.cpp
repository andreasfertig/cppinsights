#define INSIGHTS_USE_TEMPLATE

template<int val>
struct MyClass
{
    static int var;
};

template<int val> int MyClass<val>::var = val;


template<typename T, int val>
struct MyClass2
{
    static int var;
};

template<typename T, int val> int MyClass2<T, val>::var = val;


template<typename T, int val, typename... Ts>
struct MyClass3
{
    static int var;
};

template<typename T, int val, typename... Ts> int MyClass3<T, val, Ts...>::var = val;

namespace Test {
template<typename T, int val, typename... Ts>
struct MyClass3
{
    static int var;
};

template<typename T, int val, typename... Ts> int MyClass3<T, val, Ts...>::var = val;
}


int main(int argc, char* argv[])
{
    MyClass<5> a{};
    MyClass<7> b{};

    auto s = a.var + b.var;

    MyClass2<int, 5> a2{};
    MyClass2<char, 7> b2{};

    auto s2 = a2.var + b2.var;

    MyClass3<int, 5> a3{};
    MyClass3<char, 7> b3{};

    auto s3 = a3.var + b3.var;    

    Test::MyClass3<int, 5> a4{};
    Test::MyClass3<char, 7> b4{};

    auto s4 = a4.var + b4.var;    
}

