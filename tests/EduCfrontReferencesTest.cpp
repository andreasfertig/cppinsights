// cmdlineinsights:-edu-show-cfront

int& Fun(int& v)
{
    return v;
}

class A
{
    int& r;

public:
    A(int& _r)
    : r{_r}
    {
    }
};

int main()
{
    int  i{4};
    int& ri = i;

    ++ri;

    int& ru{i};
    ++ru;

    int& f = Fun(i);

    ri += 2;

    Fun(ru);

    A a{i};
}
