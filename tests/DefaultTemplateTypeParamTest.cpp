namespace Test
{
    template<typename T = void>
    struct A;

    template<>
    struct A<void>{};

    // Default param is visible here but should not be repeated
    template<class T>
    struct A {};

};

int main()
{
    Test::A<> a{};
}
