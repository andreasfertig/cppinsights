// cmdlineinsights:-edu-show-lifetime

struct Data
{
    ~Data() {}
};

struct Test
{
    Test(int) {}
    ~Test() {}

    Data get() { return {}; }
};

void Fun(Test t) {}

void FunFun(Test t, Test tt) {}

int FunFunInt(Test t, Test tt)
{
    Test t3{9};

    return 13;
}

void Fin(int) {}

void Basic()
{
    Fun(Test{4});

    FunFun(Test{2}, Test{3});
}

void Advanced()
{
    Fin(FunFunInt(Test{2}, Test{3}));
}

void WithVar()
{
    Data&& d = Test{5}.get();
}

int main()
{
    Data&& d = Test{6}.get();
}
