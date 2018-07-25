template<typename T>
class Foo
{
public:
    void Func();
};


template<>
void Foo<int>::Func()
{
}

template<>
void Foo<char>::Func()
{
}

int main()
{
    Foo<int> f;
    Foo<char> fc;
}
