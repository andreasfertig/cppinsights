// cmdlineinsights:-edu-show-cfront

class Apple
{
public:
    Apple(){};

    Apple(int x)
    : mX{x}
    {
    }

    ~Apple() { mX = 5; }

    Apple(const Apple&) {}

private:
    int mX{};
};

int main()
{
    Apple a{};
    Apple b{6};

    b = a;
}
