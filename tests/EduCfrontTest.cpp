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

    void Set(int x) { mX = x; }
    int  Get() const { return mX; }

private:
    int mX{};
};

class Other
{
    Apple mA;
    int   y;

public:
    Other(int xy)
    : mA(xy)
    , y{3}
    {
    }
};

int main()
{
    Apple a{};

    a.Set(4);

    Apple* paaa{};
    paaa->Set(5);

    Apple b{6};

    Apple c{b};

    b = a;
}
