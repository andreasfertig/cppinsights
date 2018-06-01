class Test
{
public:

    template<typename T>
    void foo(T&&){}
};

int main()
{
    Test t;

    t.foo( [&]() {} );

}
