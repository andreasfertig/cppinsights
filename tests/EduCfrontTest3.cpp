// cmdlineinsights:-edu-show-cfront

class Test
{
    int mX;

public:
    Test(int x)
    : mX{x}
    {
    }
};

Test* t = new Test{4};

class West
{
    int mX;

public:
    West(int x, double d)
    : mX{x}
    {
    }
};

West* w = new West{4, 5.6};

void Fun(Test*, int) {}

void Call()
{
    Fun(new Test{5}, 8);

    delete w;
}
