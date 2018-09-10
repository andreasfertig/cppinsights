class Test
{
public:
    Test() = default;

    template<typename T2>
    Test& operator=(const T2& other)
    {

        return *this;
    }
};

int main()
{
    Test ti;
    Test tc;

    ti = 2;

    tc = 'a';
}
