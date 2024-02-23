// cmdlineinsights:-edu-show-cfront

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
    Test t3{5};

    return 3;
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
    Data&& d = Test{4}.get();
}

int main()
{
    Data&& d = Test{4}.get();
}
