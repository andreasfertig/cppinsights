class X
{
    int i;

public:
    X(int ii = 0) ;
    void modify();
};

X::X(int ii)
{
    i = ii;
}

void X::modify()
{
    i++;
}

int main()
{
    X x;
    x.modify();
}
