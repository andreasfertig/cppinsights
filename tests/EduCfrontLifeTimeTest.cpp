// cmdlineinsights:-edu-show-cfront

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
