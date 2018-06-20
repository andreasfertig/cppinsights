int main()
{
    if(true) {
        extern void Func(int x);

        extern int Foo(double);

        float Bar(int x, float y);
    }
}

extern int Foo(double d)
{
    return 2*d;
}

float Bar(int x, float y)
{
    return x + y;
}
