// cmdlineinsights:-edu-show-lifetime

struct Test
{
    ~Test() {}
};

void Basic()
{
    Test t[2];
    int  x{2};

    ++x;
}
