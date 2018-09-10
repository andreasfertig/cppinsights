template<typename T>
class Test
{
public:
    Test() = default;

    template<typename T2>
    Test& operator=(const Test<T2>& other)
    {

        return *this;
    }
};

int main()
{
    Test<int> ti;
    Test<char> tc;

    ti = tc;

    tc = ti;
}

