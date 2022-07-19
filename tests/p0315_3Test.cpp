// cmdline:-std=c++20

namespace Test {
template<typename T>
concept X = requires(T t)
{
    []() -> void { }();  // this gets placed inside the requires expression

    decltype([]() { }){}; 
};

}

template<Test::X t>
void Fun(){}

int main()
{
    Fun<int>();
}


