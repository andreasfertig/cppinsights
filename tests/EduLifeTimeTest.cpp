// cmdlineinsights:-edu-show-cfront

struct Test
{
    ~Test() {}
};

void Basic()
{
    Test t;
    int  x{2};

    ++x;
}

void Simple()
{
    Test t;
    int  x{2};

    if(x) {
    } else {
    }
}

void Next()
{
    Test t;
    int  x{2};

    if(x) {
        Test t2;
        ++x;
    } else {
        Test t3;
        ++x;
    }
}

void Fun()
{
    Test  t;
    Test* t2;

    int x = 2;

    ++x;

    if(x) {
        Test y{};
        return;
    }

    if(x % 2) {
        return;
    } else {
        ++x;
    }

    ++x;
}

int Fun2()
{
    Test  t;
    Test* t2;

    int x = 2;

    ++x;

    if(x) {
        Test y{};
        return 3;
    }

    if(x % 2) {
        return 3;
    } else {
        ++x;
    }

    ++x;

    return 3;
}
